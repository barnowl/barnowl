/*
 * rxqueue.c
 *
 * This file contains the management routines for the receive
 * (incoming packet) queue.  The actual packet handlers are in
 * aim_rxhandlers.c.
 */

#define FAIM_INTERNAL
#include <aim.h> 

#ifndef _WIN32
#include <sys/socket.h>
#endif

/*
 *
 */
faim_internal int aim_recv(int fd, void *buf, size_t count)
{
	int left, cur; 

	for (cur = 0, left = count; left; ) {
		int ret;
		
		ret = recv(fd, ((unsigned char *)buf)+cur, left, 0);

		/* Of course EOF is an error, only morons disagree with that. */
		if (ret <= 0)
			return -1;

		cur += ret;
		left -= ret;
	}

	return cur;
}

/*
 * Read into a byte stream.  Will not read more than count, but may read
 * less if there is not enough room in the stream buffer.
 */
faim_internal int aim_bstream_recv(aim_bstream_t *bs, int fd, size_t count)
{
	int red = 0;

	if (!bs || (fd < 0) || (count < 0))
		return -1;
	
	if (count > (bs->len - bs->offset))
		count = bs->len - bs->offset; /* truncate to remaining space */

	if (count) {

		red = aim_recv(fd, bs->data + bs->offset, count);

		if (red <= 0)
			return -1;
	}

	bs->offset += red;

	return red;
}

/**
 * aim_frame_destroy - free aim_frame_t
 * @frame: the frame to free
 *
 * returns -1 on error; 0 on success.
 *
 */
faim_internal void aim_frame_destroy(aim_frame_t *frame)
{

	free(frame->data.data); /* XXX aim_bstream_free */
	free(frame);

	return;
}

/*
 * Read a FLAP header from conn into fr, and return the number of bytes in the payload.
 */
static faim_shortfunc int aim_get_command_flap(aim_session_t *sess, aim_conn_t *conn, aim_frame_t *fr)
{
	fu8_t flaphdr_raw[6];
	aim_bstream_t flaphdr;
	fu16_t payloadlen;
	
	aim_bstream_init(&flaphdr, flaphdr_raw, sizeof(flaphdr_raw));

	/*
	 * Read FLAP header.  Six bytes:
	 *   0 char  -- Always 0x2a
	 *   1 char  -- Channel ID.  Usually 2 -- 1 and 4 are used during login.
	 *   2 short -- Sequence number
	 *   4 short -- Number of data bytes that follow.
	 */
	if (aim_bstream_recv(&flaphdr, conn->fd, 6) < 6) {
		aim_conn_close(conn);
		return -1;
	}

	aim_bstream_rewind(&flaphdr);

	/*
	 * This shouldn't happen unless the socket breaks, the server breaks,
	 * or we break.  We must handle it just in case.
	 */
	if (aimbs_get8(&flaphdr) != 0x2a) {
		fu8_t start;

		aim_bstream_rewind(&flaphdr);
		start = aimbs_get8(&flaphdr);
		faimdprintf(sess, 0, "FLAP framing disrupted (0x%02x)", start);
		aim_conn_close(conn);
		return -1;
	}	

	/* we're doing FLAP if we're here */
	fr->hdrtype = AIM_FRAMETYPE_FLAP;

	fr->hdr.flap.type = aimbs_get8(&flaphdr);
	fr->hdr.flap.seqnum = aimbs_get16(&flaphdr);
	payloadlen = aimbs_get16(&flaphdr); /* length of payload */

	return payloadlen;
}

/*
 * Read a rendezvous header from conn into fr, and return the number of bytes in the payload.
 */
static int aim_get_command_rendezvous(aim_session_t *sess, aim_conn_t *conn, aim_frame_t *fr)
{
	fu8_t rendhdr_raw[8];
	aim_bstream_t rendhdr;

	aim_bstream_init(&rendhdr, rendhdr_raw, sizeof(rendhdr_raw));

	if (aim_bstream_recv(&rendhdr, conn->fd, 8) < 8) {
		aim_conn_close(conn);
		return -1;
	}

	aim_bstream_rewind(&rendhdr);

	fr->hdrtype = AIM_FRAMETYPE_OFT; /* a misnomer--rendezvous */

	aimbs_getrawbuf(&rendhdr, fr->hdr.rend.magic, 4);
	fr->hdr.rend.hdrlen = aimbs_get16(&rendhdr) - 8;
	fr->hdr.rend.type = aimbs_get16(&rendhdr);

	return fr->hdr.rend.hdrlen;
}

/*
 * Grab a single command sequence off the socket, and enqueue it in the incoming event queue 
 * in a separate struct.
 */
faim_export int aim_get_command(aim_session_t *sess, aim_conn_t *conn)
{
	aim_frame_t *newrx;
	fu16_t payloadlen;

	if (!sess || !conn)
		return -1;

	if (conn->fd == -1)
		return -1; /* it's an aim_conn_close()'d connection */

	if (conn->fd < 3) /* can happen when people abuse the interface */
		return -1;

	if (conn->status & AIM_CONN_STATUS_INPROGRESS)
		return aim_conn_completeconnect(sess, conn);

	if (!(newrx = (aim_frame_t *)calloc(sizeof(aim_frame_t), 1)))
		return -1;

	/*
	 * Rendezvous (client to client) connections do not speak FLAP, so this 
	 * function will break on them.
	 */
	if (conn->type == AIM_CONN_TYPE_RENDEZVOUS)
		payloadlen = aim_get_command_rendezvous(sess, conn, newrx);
	else if (conn->type == AIM_CONN_TYPE_RENDEZVOUS_OUT) {
		faimdprintf(sess, 0, "AIM_CONN_TYPE_RENDEZVOUS_OUT on fd %d\n", conn->fd);
		free(newrx);
		return -1;
	} else
		payloadlen = aim_get_command_flap(sess, conn, newrx);

	newrx->nofree = 0; /* free by default */

	if (payloadlen) {
		fu8_t *payload = NULL;

		if (!(payload = (fu8_t *) malloc(payloadlen))) {
			aim_frame_destroy(newrx);
			return -1;
		}

		aim_bstream_init(&newrx->data, payload, payloadlen);

		/* read the payload */
		if (aim_bstream_recv(&newrx->data, conn->fd, payloadlen) < payloadlen) {
			aim_frame_destroy(newrx); /* free's payload */
			aim_conn_close(conn);
			return -1;
		}
	} else
		aim_bstream_init(&newrx->data, NULL, 0);


	aim_bstream_rewind(&newrx->data);

	newrx->conn = conn;

	newrx->next = NULL;  /* this will always be at the bottom */

	if (!sess->queue_incoming)
		sess->queue_incoming = newrx;
	else {
		aim_frame_t *cur;

		for (cur = sess->queue_incoming; cur->next; cur = cur->next)
			;
		cur->next = newrx;
	}

	newrx->conn->lastactivity = time(NULL);

	return 0;  
}

/*
 * Purge recieve queue of all handled commands (->handled==1).  Also
 * allows for selective freeing using ->nofree so that the client can
 * keep the data for various purposes.  
 *
 * If ->nofree is nonzero, the frame will be delinked from the global list, 
 * but will not be free'ed.  The client _must_ keep a pointer to the
 * data -- libfaim will not!  If the client marks ->nofree but
 * does not keep a pointer, it's lost forever.
 *
 */
faim_export void aim_purge_rxqueue(aim_session_t *sess)
{
	aim_frame_t *cur, **prev;

	for (prev = &sess->queue_incoming; (cur = *prev); ) {
		if (cur->handled) {

			*prev = cur->next;
			
			if (!cur->nofree)
				aim_frame_destroy(cur);

		} else
			prev = &cur->next;
	}

	return;
}

/*
 * Since aim_get_command will aim_conn_kill dead connections, we need
 * to clean up the rxqueue of unprocessed connections on that socket.
 *
 * XXX: this is something that was handled better in the old connection
 * handling method, but eh.
 */
faim_internal void aim_rxqueue_cleanbyconn(aim_session_t *sess, aim_conn_t *conn)
{
	aim_frame_t *currx;

	for (currx = sess->queue_incoming; currx; currx = currx->next) {
		if ((!currx->handled) && (currx->conn == conn))
			currx->handled = 1;
	}	
	return;
}
