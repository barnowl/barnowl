/*
 * txqueue.c
 *
 * Herein lies all the mangement routines for the transmit (Tx) queue.
 *
 */

#define FAIM_INTERNAL
#include <aim.h>

#ifndef _WIN32
#include <sys/socket.h>
#else
#include "win32dep.h"
#endif

/*
 * Allocate a new tx frame.
 *
 * This is more for looks than anything else.
 *
 * Right now, that is.  If/when we implement a pool of transmit
 * frames, this will become the request-an-unused-frame part.
 *
 * framing = AIM_FRAMETYPE_OFT/FLAP
 * chan = channel for FLAP, hdrtype for OFT
 *
 */
faim_internal aim_frame_t *aim_tx_new(aim_session_t *sess, aim_conn_t *conn, fu8_t framing, fu16_t chan, int datalen)
{
	aim_frame_t *fr;

	if (!conn) {
		faimdprintf(sess, 0, "aim_tx_new: ERROR: no connection specified\n");
		return NULL;
	}

	/* For sanity... */
	if ((conn->type == AIM_CONN_TYPE_RENDEZVOUS) || (conn->type == AIM_CONN_TYPE_LISTENER)) {
		if (framing != AIM_FRAMETYPE_OFT) {
			faimdprintf(sess, 0, "aim_tx_new: attempted to allocate inappropriate frame type for rendezvous connection\n");
			return NULL;
		}
	} else {
		if (framing != AIM_FRAMETYPE_FLAP) {
			faimdprintf(sess, 0, "aim_tx_new: attempted to allocate inappropriate frame type for FLAP connection\n");
			return NULL;
		}
	}

	if (!(fr = (aim_frame_t *)malloc(sizeof(aim_frame_t))))
		return NULL;
	memset(fr, 0, sizeof(aim_frame_t));

	fr->conn = conn; 

	fr->hdrtype = framing;

	if (fr->hdrtype == AIM_FRAMETYPE_FLAP) {

		fr->hdr.flap.type = chan;

	} else if (fr->hdrtype == AIM_FRAMETYPE_OFT) {

		fr->hdr.rend.type = chan;

	} else 
		faimdprintf(sess, 0, "tx_new: unknown framing\n");

	if (datalen > 0) {
		fu8_t *data;

		if (!(data = (unsigned char *)malloc(datalen))) {
			aim_frame_destroy(fr);
			return NULL;
		}

		aim_bstream_init(&fr->data, data, datalen);
	}

	return fr;
}

/*
 * aim_tx_enqeue__queuebased()
 *
 * The overall purpose here is to enqueue the passed in command struct
 * into the outgoing (tx) queue.  Basically...
 *   1) Make a scope-irrelevent copy of the struct
 *   3) Mark as not-sent-yet
 *   4) Enqueue the struct into the list
 *   6) Return
 *
 * Note that this is only used when doing queue-based transmitting;
 * that is, when sess->tx_enqueue is set to &aim_tx_enqueue__queuebased.
 *
 */
static int aim_tx_enqueue__queuebased(aim_session_t *sess, aim_frame_t *fr)
{

	if (!fr->conn) {
		faimdprintf(sess, 1, "aim_tx_enqueue: WARNING: enqueueing packet with no connecetion\n");
		fr->conn = aim_getconn_type(sess, AIM_CONN_TYPE_BOS);
	}

	if (fr->hdrtype == AIM_FRAMETYPE_FLAP) {
		/* assign seqnum -- XXX should really not assign until hardxmit */
		fr->hdr.flap.seqnum = aim_get_next_txseqnum(fr->conn);
	}

	fr->handled = 0; /* not sent yet */

	/* see overhead note in aim_rxqueue counterpart */
	if (!sess->queue_outgoing)
		sess->queue_outgoing = fr;
	else {
		aim_frame_t *cur;

		for (cur = sess->queue_outgoing; cur->next; cur = cur->next)
			;
		cur->next = fr;
	}

	return 0;
}

/*
 * aim_tx_enqueue__immediate()
 *
 * Parallel to aim_tx_enqueue__queuebased, however, this bypasses
 * the whole queue mess when you want immediate writes to happen.
 *
 * Basically the same as its __queuebased couterpart, however
 * instead of doing a list append, it just calls aim_tx_sendframe()
 * right here. 
 * 
 */
static int aim_tx_enqueue__immediate(aim_session_t *sess, aim_frame_t *fr)
{

	if (!fr->conn) {
		faimdprintf(sess, 1, "aim_tx_enqueue: ERROR: packet has no connection\n");
		aim_frame_destroy(fr);
		return 0;
	}

	if (fr->hdrtype == AIM_FRAMETYPE_FLAP)
		fr->hdr.flap.seqnum = aim_get_next_txseqnum(fr->conn);

	fr->handled = 0; /* not sent yet */

	aim_tx_sendframe(sess, fr);

	aim_frame_destroy(fr);

	return 0;
}

faim_export int aim_tx_setenqueue(aim_session_t *sess, int what, int (*func)(aim_session_t *, aim_frame_t *))
{
	
	if (what == AIM_TX_QUEUED)
		sess->tx_enqueue = &aim_tx_enqueue__queuebased;
	else if (what == AIM_TX_IMMEDIATE) 
		sess->tx_enqueue = &aim_tx_enqueue__immediate;
	else if (what == AIM_TX_USER) {
		if (!func)
			return -EINVAL;
		sess->tx_enqueue = func;
	} else
		return -EINVAL; /* unknown action */

	return 0;
}

faim_internal int aim_tx_enqueue(aim_session_t *sess, aim_frame_t *fr)
{
	
	/*
	 * If we want to send a connection thats inprogress, we have to force
	 * them to use the queue based version. Otherwise, use whatever they
	 * want.
	 */
	if (fr && fr->conn && 
			(fr->conn->status & AIM_CONN_STATUS_INPROGRESS)) {
		return aim_tx_enqueue__queuebased(sess, fr);
	}

	return (*sess->tx_enqueue)(sess, fr);
}

/* 
 *  aim_get_next_txseqnum()
 *
 *   This increments the tx command count, and returns the seqnum
 *   that should be stamped on the next FLAP packet sent.  This is
 *   normally called during the final step of packet preparation
 *   before enqueuement (in aim_tx_enqueue()).
 *
 */
faim_internal flap_seqnum_t aim_get_next_txseqnum(aim_conn_t *conn)
{
	flap_seqnum_t ret;
	
	ret = ++conn->seqnum;

	return ret;
}

static int aim_send(int fd, const void *buf, size_t count)
{
	int left, cur;

	for (cur = 0, left = count; left; ) {
		int ret;

		ret = send(fd, ((unsigned char *)buf)+cur, left, 0);
		if (ret == -1)
			return -1;
		else if (ret == 0)
			return cur;

		cur += ret;
		left -= ret;
	}

	return cur;
}

static int aim_bstream_send(aim_bstream_t *bs, aim_conn_t *conn, size_t count)
{
	int wrote = 0;
	if (!bs || !conn || (count < 0))
		return -EINVAL;

	if (count > aim_bstream_empty(bs))
		count = aim_bstream_empty(bs); /* truncate to remaining space */

	if (count) {
		if ((conn->type == AIM_CONN_TYPE_RENDEZVOUS) && 
		    (conn->subtype == AIM_CONN_SUBTYPE_OFT_DIRECTIM)) {
			/* I strongly suspect that this is a horrible thing to do
			 * and I feel really guilty doing it. */
			const char *sn = aim_odc_getsn(conn);
			aim_rxcallback_t userfunc;
			while (count - wrote > 1024) {
				wrote = wrote + aim_send(conn->fd, bs->data + bs->offset + wrote, 1024);
				if ((userfunc=aim_callhandler(conn->sessv, conn, 
								AIM_CB_FAM_SPECIAL, 
								AIM_CB_SPECIAL_IMAGETRANSFER)))
				  userfunc(conn->sessv, NULL, sn, 
					   count-wrote>1024 ? ((double)wrote / count) : 1);
			}
		}
		if (count - wrote) {
			wrote = wrote + aim_send(conn->fd, bs->data + bs->offset + wrote, count - wrote);
		}

	}

	if (((aim_session_t *)conn->sessv)->debug >= 2) {
		int i;
		aim_session_t *sess = (aim_session_t *)conn->sessv;

		faimdprintf(sess, 2, "\nOutgoing data: (%d bytes)", wrote);
		for (i = 0; i < wrote; i++) {
			if (!(i % 8)) 
				faimdprintf(sess, 2, "\n\t");
			faimdprintf(sess, 2, "0x%02x ", *(bs->data + bs->offset + i));
		}
		faimdprintf(sess, 2, "\n");
	}

	bs->offset += wrote;

	return wrote;
}

static int sendframe_flap(aim_session_t *sess, aim_frame_t *fr)
{
	aim_bstream_t obs;
	fu8_t *obs_raw;
	int payloadlen, err = 0, obslen;

	payloadlen = aim_bstream_curpos(&fr->data);

	if (!(obs_raw = malloc(6 + payloadlen)))
		return -ENOMEM;

	aim_bstream_init(&obs, obs_raw, 6 + payloadlen);

	/* FLAP header */
	aimbs_put8(&obs, 0x2a);
	aimbs_put8(&obs, fr->hdr.flap.type);
	aimbs_put16(&obs, fr->hdr.flap.seqnum);
	aimbs_put16(&obs, payloadlen);

	/* payload */
	aim_bstream_rewind(&fr->data);
	aimbs_putbs(&obs, &fr->data, payloadlen);

	obslen = aim_bstream_curpos(&obs);
	aim_bstream_rewind(&obs);
	if (aim_bstream_send(&obs, fr->conn, obslen) != obslen)
		err = -errno;
	
	free(obs_raw); /* XXX aim_bstream_free */

	fr->handled = 1;
	fr->conn->lastactivity = time(NULL);

	return err;
}

static int sendframe_rendezvous(aim_session_t *sess, aim_frame_t *fr)
{
	aim_bstream_t bs;
	fu8_t *bs_raw;
	int err = 0;
	int totlen = 8 + aim_bstream_curpos(&fr->data);

	if (!(bs_raw = malloc(totlen)))
		return -1;

	aim_bstream_init(&bs, bs_raw, totlen);

	aimbs_putraw(&bs, fr->hdr.rend.magic, 4);
	aimbs_put16(&bs, 8 + fr->hdr.rend.hdrlen);
	aimbs_put16(&bs, fr->hdr.rend.type);

	/* payload */
	aim_bstream_rewind(&fr->data);
	aimbs_putbs(&bs, &fr->data, totlen - 8);

	aim_bstream_rewind(&bs);

	if (aim_bstream_send(&bs, fr->conn, totlen) != totlen)
		err = -errno;

	free(bs_raw); /* XXX aim_bstream_free */

	fr->handled = 1;
	fr->conn->lastactivity = time(NULL);

	return err;
}

faim_internal int aim_tx_sendframe(aim_session_t *sess, aim_frame_t *fr)
{
	if (fr->hdrtype == AIM_FRAMETYPE_FLAP)
		return sendframe_flap(sess, fr);
	else if (fr->hdrtype == AIM_FRAMETYPE_OFT)
		return sendframe_rendezvous(sess, fr);
	return -1;
}

faim_export int aim_tx_flushqueue(aim_session_t *sess)
{
	aim_frame_t *cur;

	for (cur = sess->queue_outgoing; cur; cur = cur->next) {

		if (cur->handled)
	       		continue; /* already been sent */

		if (cur->conn && (cur->conn->status & AIM_CONN_STATUS_INPROGRESS))
			continue;

		/*
		 * And now for the meager attempt to force transmit
		 * latency and avoid missed messages.
		 */
		if ((cur->conn->lastactivity + cur->conn->forcedlatency) >= time(NULL)) {
			/* 
			 * XXX should be a break! we dont want to block the 
			 * upper layers
			 *
			 * XXX or better, just do this right.
			 *
			 */
			sleep((cur->conn->lastactivity + cur->conn->forcedlatency) - time(NULL));
		}

		/* XXX this should call the custom "queuing" function!! */
		aim_tx_sendframe(sess, cur);
	}

	/* purge sent commands from queue */
	aim_tx_purgequeue(sess);

	return 0;
}

/*
 *  aim_tx_purgequeue()
 *  
 *  This is responsable for removing sent commands from the transmit 
 *  queue. This is not a required operation, but it of course helps
 *  reduce memory footprint at run time!  
 *
 */
faim_export void aim_tx_purgequeue(aim_session_t *sess)
{
	aim_frame_t *cur, **prev;

	for (prev = &sess->queue_outgoing; (cur = *prev); ) {

		if (cur->handled) {
			*prev = cur->next;

			aim_frame_destroy(cur);

		} else
			prev = &cur->next;
	}

	return;
}

/**
 * aim_tx_cleanqueue - get rid of packets waiting for tx on a dying conn
 * @sess: session
 * @conn: connection that's dying
 *
 * for now this simply marks all packets as sent and lets them
 * disappear without warning.
 *
 */
faim_internal void aim_tx_cleanqueue(aim_session_t *sess, aim_conn_t *conn)
{
	aim_frame_t *cur;

	for (cur = sess->queue_outgoing; cur; cur = cur->next) {
		if (cur->conn == conn)
			cur->handled = 1;
	}

	return;
}
