/*
 *  aim_rxqueue.c
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

faim_internal int aim_bstream_init(aim_bstream_t *bs, fu8_t *data, int len)
{
	
	if (!bs)
		return -1;

	bs->data = data;
	bs->len = len;
	bs->offset = 0;

	return 0;
}

faim_internal int aim_bstream_empty(aim_bstream_t *bs)
{
	return bs->len - bs->offset;
}

faim_internal int aim_bstream_curpos(aim_bstream_t *bs)
{
	return bs->offset;
}

faim_internal int aim_bstream_setpos(aim_bstream_t *bs, int off)
{

	if (off > bs->len)
		return -1;

	bs->offset = off;

	return off;
}

faim_internal void aim_bstream_rewind(aim_bstream_t *bs)
{

	aim_bstream_setpos(bs, 0);

	return;
}

faim_internal int aim_bstream_advance(aim_bstream_t *bs, int n)
{

	if (aim_bstream_empty(bs) < n)
		return 0; /* XXX throw an exception */

	bs->offset += n;

	return n;
}

faim_internal fu8_t aimbs_get8(aim_bstream_t *bs)
{
	
	if (aim_bstream_empty(bs) < 1)
		return 0; /* XXX throw an exception */
	
	bs->offset++;
	
	return aimutil_get8(bs->data + bs->offset - 1);
}

faim_internal fu16_t aimbs_get16(aim_bstream_t *bs)
{
	
	if (aim_bstream_empty(bs) < 2)
		return 0; /* XXX throw an exception */
	
	bs->offset += 2;
	
	return aimutil_get16(bs->data + bs->offset - 2);
}

faim_internal fu32_t aimbs_get32(aim_bstream_t *bs)
{
	
	if (aim_bstream_empty(bs) < 4)
		return 0; /* XXX throw an exception */
	
	bs->offset += 4;
	
	return aimutil_get32(bs->data + bs->offset - 4);
}

faim_internal fu8_t aimbs_getle8(aim_bstream_t *bs)
{
	
	if (aim_bstream_empty(bs) < 1)
		return 0; /* XXX throw an exception */
	
	bs->offset++;
	
	return aimutil_getle8(bs->data + bs->offset - 1);
}

faim_internal fu16_t aimbs_getle16(aim_bstream_t *bs)
{
	
	if (aim_bstream_empty(bs) < 2)
		return 0; /* XXX throw an exception */
	
	bs->offset += 2;
	
	return aimutil_getle16(bs->data + bs->offset - 2);
}

faim_internal fu32_t aimbs_getle32(aim_bstream_t *bs)
{
	
	if (aim_bstream_empty(bs) < 4)
		return 0; /* XXX throw an exception */
	
	bs->offset += 4;
	
	return aimutil_getle32(bs->data + bs->offset - 4);
}

faim_internal int aimbs_put8(aim_bstream_t *bs, fu8_t v)
{

	if (aim_bstream_empty(bs) < 1)
		return 0; /* XXX throw an exception */

	bs->offset += aimutil_put8(bs->data + bs->offset, v);

	return 1;
}

faim_internal int aimbs_put16(aim_bstream_t *bs, fu16_t v)
{

	if (aim_bstream_empty(bs) < 2)
		return 0; /* XXX throw an exception */

	bs->offset += aimutil_put16(bs->data + bs->offset, v);

	return 2;
}

faim_internal int aimbs_put32(aim_bstream_t *bs, fu32_t v)
{

	if (aim_bstream_empty(bs) < 4)
		return 0; /* XXX throw an exception */

	bs->offset += aimutil_put32(bs->data + bs->offset, v);

	return 1;
}

faim_internal int aimbs_putle8(aim_bstream_t *bs, fu8_t v)
{

	if (aim_bstream_empty(bs) < 1)
		return 0; /* XXX throw an exception */

	bs->offset += aimutil_putle8(bs->data + bs->offset, v);

	return 1;
}

faim_internal int aimbs_putle16(aim_bstream_t *bs, fu16_t v)
{

	if (aim_bstream_empty(bs) < 2)
		return 0; /* XXX throw an exception */

	bs->offset += aimutil_putle16(bs->data + bs->offset, v);

	return 2;
}

faim_internal int aimbs_putle32(aim_bstream_t *bs, fu32_t v)
{

	if (aim_bstream_empty(bs) < 4)
		return 0; /* XXX throw an exception */

	bs->offset += aimutil_putle32(bs->data + bs->offset, v);

	return 1;
}

faim_internal int aimbs_getrawbuf(aim_bstream_t *bs, fu8_t *buf, int len)
{

	if (aim_bstream_empty(bs) < len)
		return 0;

	memcpy(buf, bs->data + bs->offset, len);
	bs->offset += len;

	return len;
}

faim_internal fu8_t *aimbs_getraw(aim_bstream_t *bs, int len)
{
	fu8_t *ob;

	if (!(ob = malloc(len)))
		return NULL;

	if (aimbs_getrawbuf(bs, ob, len) < len) {
		free(ob);
		return NULL;
	}

	return ob;
}

faim_internal char *aimbs_getstr(aim_bstream_t *bs, int len)
{
	char *ob;

	if (!(ob = malloc(len+1)))
		return NULL;

	if (aimbs_getrawbuf(bs, ob, len) < len) {
		free(ob);
		return NULL;
	}
	
	ob[len] = '\0';

	return ob;
}

faim_internal int aimbs_putraw(aim_bstream_t *bs, const fu8_t *v, int len)
{

	if (aim_bstream_empty(bs) < len)
		return 0; /* XXX throw an exception */

	memcpy(bs->data + bs->offset, v, len);
	bs->offset += len;

	return len;
}

faim_internal int aimbs_putbs(aim_bstream_t *bs, aim_bstream_t *srcbs, int len)
{

	if (aim_bstream_empty(srcbs) < len)
		return 0; /* XXX throw exception (underrun) */

	if (aim_bstream_empty(bs) < len)
		return 0; /* XXX throw exception (overflow) */

	memcpy(bs->data + bs->offset, srcbs->data + srcbs->offset, len);
	bs->offset += len;
	srcbs->offset += len;

	return len;
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

	if (frame->hdrtype == AIM_FRAMETYPE_OFT)
		free(frame->hdr.oft.hdr2);
	free(frame);
	
	return;
} 


/*
 * Grab a single command sequence off the socket, and enqueue
 * it in the incoming event queue in a seperate struct.
 */
faim_export int aim_get_command(aim_session_t *sess, aim_conn_t *conn)
{
	fu8_t flaphdr_raw[6];
	aim_bstream_t flaphdr;
	aim_frame_t *newrx;
	fu16_t payloadlen;
	
	if (!sess || !conn)
		return 0;

	if (conn->fd == -1)
		return -1; /* its a aim_conn_close()'d connection */

	if (conn->fd < 3)  /* can happen when people abuse the interface */
		return 0;

	if (conn->status & AIM_CONN_STATUS_INPROGRESS)
		return aim_conn_completeconnect(sess, conn);

	/*
	 * Rendezvous (client-client) connections do not speak
	 * FLAP, so this function will break on them.
	 */
	if (conn->type == AIM_CONN_TYPE_RENDEZVOUS) 
		return aim_get_command_rendezvous(sess, conn);
	else if (conn->type == AIM_CONN_TYPE_RENDEZVOUS_OUT) {
		faimdprintf(sess, 0, "AIM_CONN_TYPE_RENDEZVOUS_OUT on fd %d\n", conn->fd);
		return 0; 
	}

	aim_bstream_init(&flaphdr, flaphdr_raw, sizeof(flaphdr_raw));

	/*
	 * Read FLAP header.  Six bytes:
	 *    
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

	/* allocate a new struct */
	if (!(newrx = (aim_frame_t *)malloc(sizeof(aim_frame_t))))
		return -1;
	memset(newrx, 0, sizeof(aim_frame_t));

	/* we're doing FLAP if we're here */
	newrx->hdrtype = AIM_FRAMETYPE_FLAP;
	
	newrx->hdr.flap.type = aimbs_get8(&flaphdr);
	newrx->hdr.flap.seqnum = aimbs_get16(&flaphdr);
	payloadlen = aimbs_get16(&flaphdr);

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

