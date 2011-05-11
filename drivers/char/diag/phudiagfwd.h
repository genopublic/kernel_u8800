#ifndef PHUDIAGFWD_H
#define PHUDIAGFWD_H
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>

#define PHU_DIAG_IN_BUF_SIZE 1024*128

#define PHU_DIAG_OUT_BUF_SIZE 1024*32
#define PHU_DIAG_SMD_BUF_MAX 1024*4
#define PHU_DIAG_READ_WRITE_BUF_SIZE 1024*4
#define PHU_DIAG_SEND_PACKET_MAX_SIZE 1024*4

struct phudiag_buf{
	int data_length;
	int used;
	int buf_size;
	uint8_t buf[0];
};

struct phudiag_ring_buf{
	uint8_t * start;
	uint8_t * end;
	int buf_size;
	uint8_t buf[0];
};


struct phudiag_ring_buf * phudiag_ring_buf_malloc(int buf_size);
void phudiag_ring_buf_free(struct phudiag_ring_buf *ring_buf);

int phudiagfwd_ring_buf_get_data_length(struct phudiag_ring_buf *ring_buf);
int phudiagfwd_ring_buf_is_empty(struct phudiag_ring_buf *ring_buf);
int phudiagfwd_ring_buf_get_free_size(struct phudiag_ring_buf *ring_buf );
int phudiagfwd_ring_buf_set_data(struct phudiag_ring_buf *ring_buf ,uint8_t* data,int data_length);
int phudiagfwd_ring_buf_get_data(struct phudiag_ring_buf *ring_buf,uint8_t* buf,int buf_size);
int phudiagfwd_ring_buf_user_get_data(struct phudiag_ring_buf *ring_buf,char __user *buf,int buf_size);

int phudiagfwd_ring_buf2_is_empty(struct phudiag_ring_buf *ring_buf);
int phudiagfwd_ring_buf2_get_free_size(struct phudiag_ring_buf *ring_buf );
int phudiagfwd_ring_buf2_set_data(struct phudiag_ring_buf *ring_buf ,uint8_t* data,int data_length);
int phudiagfwd_ring_buf2_get_data(struct phudiag_ring_buf *ring_buf,uint8_t* buf,int buf_size);
int phudiagfwd_ring_buf2_user_set_data(struct phudiag_ring_buf *ring_buf ,const char __user * data,int data_length);

int phudiagfwd_write_data_to_in_buf(uint8_t* buf, int length);
void phudiagfwd_in_buf_change_busy_state(int smd_avail_data_length,int usb_buf_busy);
void phudiagfwd_write_to_smd_work_fn(struct work_struct *work);

#endif
