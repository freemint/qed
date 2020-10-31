#ifndef _qed_dd_h_
#define _qed_dd_h_

/*
 * communication via drag&drop.
 */

/*
 * The following constants are used, when something wa dragged
 * to a window of qed, and passed as src_icon in icon_drag()
 */
#define DRAGDROP_FILE	0xD01		/* a filename in drag_filename */
#define DRAGDROP_PATH	0xD02		/* a path in drag_filename */
#define DRAGDROP_DATA	0xD03		/* some data in drag_data */

/* 
 * magic value of drag_data_size: drag_data points to a RING,
 * which was loaded via D&D
*/
#define DDS_RINGP			-42

/*
 * Variables for drag operations
*/
extern PATH	drag_filename;
extern char	*drag_data;
extern long	drag_data_size;


/*
 * Handle requests.
 */
void	handle_dd	(short *msg);

/*
 * Handle D&D for global window cycling (VA_DRAGACCWIND).
*/
void	handle_avdd(short win_handle, short kstate, char *arg);

/*
 * Send the text in <t> to the window <win_id>.
 */
void send_dd(short win_id, short m_x, short m_y, short kstate, RINGP t);

#endif
