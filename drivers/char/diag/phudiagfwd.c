
#include"phudiagfwd.h"
#include"phudiagchar.h"
struct phudiag_data

{
	int length;
	uint8_t data[0];
};



struct phudiag_ring_buf * phudiag_ring_buf_malloc(int buf_size)

{
	struct phudiag_ring_buf * ring_buf;

	ring_buf =  kzalloc(sizeof(struct phudiag_ring_buf) + buf_size, GFP_KERNEL);
	if(ring_buf)
	{
		ring_buf->buf_size = buf_size;
		ring_buf->start = ring_buf->end = ring_buf->buf;
		#ifdef PHUDIAG_DEBUG
		printk("phudiagchar :phudiag_ring_buf_malloc  success! malloc mem size = 0x%x\n",buf_size);
		#endif
	}
	else
	{
		printk("phudiagchar :phudiag_ring_buf_malloc  fail! malloc mem size = 0x%x\n",buf_size);
	}
	return ring_buf;
}



void phudiag_ring_buf_free(struct phudiag_ring_buf *ring_buf)

{
	if(ring_buf)
	{
		kfree(ring_buf);
	}
}


int phudiagfwd_ring_buf_is_empty(struct phudiag_ring_buf *ring_buf)
{
	if(!ring_buf)
	{
		return -1;
	}
	
	return !(ring_buf->start - ring_buf->end);
}
int phudiagfwd_ring_buf_get_data_length(struct phudiag_ring_buf *ring_buf)
{
	if(!ring_buf)
	{
		return -1;
	}
	
	return ring_buf->buf_size - phudiagfwd_ring_buf_get_free_size(ring_buf);		
}

int phudiagfwd_ring_buf_get_free_size(struct phudiag_ring_buf *ring_buf )
{
	uint8_t* buf_end;
	int free_buf_size = 0;



	if(!ring_buf)
	{
		return -1;
	}
	
	buf_end = ring_buf->buf + ring_buf->buf_size;

	if(ring_buf->start <=  ring_buf->end)
	{
		free_buf_size = ring_buf->buf_size - (ring_buf->end - ring_buf->start);
	}
	else
	{
		free_buf_size =  ring_buf->start -ring_buf->end;
	}
	#ifdef PHUDIAG_DEBUG
	#endif
	
	return free_buf_size;

}



int phudiagfwd_ring_buf_set_data(struct phudiag_ring_buf *ring_buf ,uint8_t* data,int data_length)

{
	int buffer_size = 0;
	int copyed_size = 0;
	uint8_t* buf_end;



	if(!ring_buf)

	{
		return -1;
	}


	if(!data || 0 == data_length )
	{
		return -1;
	}
	
	#ifdef PHUDIAG_DEBUG
	#endif


	buf_end = ring_buf->buf + ring_buf->buf_size;
	buffer_size = buf_end - ring_buf->end;


	if(data_length > buffer_size)
	{	
		if(buffer_size)
		{
			memcpy(ring_buf->end,data,buffer_size);
		}
		copyed_size = buffer_size;
		data_length -= buffer_size;
		memcpy(ring_buf->buf,data + copyed_size,data_length);
		ring_buf->end = ring_buf->buf + data_length;
	}
	else
	{
		memcpy(ring_buf->end,data,data_length);
		ring_buf->end = ring_buf->end + data_length;
	}
	
	#ifdef PHUDIAG_DEBUG
	#endif
	
	return data_length;
}



int phudiagfwd_ring_buf_get_data(struct phudiag_ring_buf *ring_buf,uint8_t* buf,int buf_size)

{
	int copy_length = 0;
	int data_length = 0;
	uint8_t* buf_end;



	if(!ring_buf)
	{
		return -1;
	}



	if(!buf || 0 == buf_size )
	{
		return -1;
	}


	buf_end = ring_buf->buf + ring_buf->buf_size;

	data_length = phudiagfwd_ring_buf_get_data_length(ring_buf);
	
	#ifdef PHUDIAG_DEBUG
	#endif
	
	if(data_length > buf_size)
	{
		data_length = buf_size;
	}

	if(buf_end - ring_buf->start >=  data_length)
	{
		memcpy(buf,ring_buf->start,data_length);
		ring_buf->start += data_length;
	}
	else
	{
		copy_length = buf_end - ring_buf->start;
		if(copy_length)
		{
			memcpy(buf,ring_buf->start,copy_length);
		}

		memcpy(buf + copy_length ,ring_buf->buf,(data_length - copy_length));
		ring_buf->start = ring_buf->buf + (data_length - copy_length);
	}
	
	#ifdef PHUDIAG_DEBUG
	#endif

	return data_length;
}

int phudiagfwd_ring_buf_user_get_data(struct phudiag_ring_buf *ring_buf,char __user *buf,int buf_size)

{
	int copy_length = 0;
	int data_length = 0;
	uint8_t* buf_end;



	if(!ring_buf)
	{
		return -1;
	}



	if(!buf || 0 == buf_size )
	{
		return -1;
	}


	buf_end = ring_buf->buf + ring_buf->buf_size;

	data_length = phudiagfwd_ring_buf_get_data_length(ring_buf);
	
	#ifdef PHUDIAG_DEBUG
	#endif

	if(data_length > buf_size)

	{
		data_length = buf_size;
	}


	if(buf_end - ring_buf->start >=  data_length)
	{
		if(copy_to_user(buf,(void*)ring_buf->start,data_length))
		{
			return -1;
		}
		ring_buf->start += data_length;
	}

	else

	{
		copy_length = buf_end - ring_buf->start;

		if(copy_length)
		{
			if(copy_to_user(buf,(void*)ring_buf->start,copy_length))
			{
				return -1;
			}
		}

		if(copy_to_user(buf + copy_length ,(void*)ring_buf->buf,(data_length - copy_length)))
		{
			return -1;
		}
		ring_buf->start = ring_buf->buf + (data_length - copy_length);
	}
	
	#ifdef PHUDIAG_DEBUG
	#endif
	
	return data_length;
}

//////////////////////////////
int phudiagfwd_ring_buf2_is_empty(struct phudiag_ring_buf *ring_buf)

{
	uint8_t* buf_end;
	int is_empty = 0;

	if(!ring_buf)
	{
		return -1;
	}

	buf_end = ring_buf->buf + ring_buf->buf_size;

	if(ring_buf->start <=  ring_buf->end)
	{
		if(ring_buf->end - ring_buf->start <= sizeof(int))
		{
			is_empty = 1;
		}
	}
	else
	{
		if(buf_end -ring_buf->start < sizeof(int))
		{
			if(ring_buf->end - ring_buf->buf <= sizeof(int))
			{
				is_empty = 1;
			}
		}
		else if(buf_end -ring_buf->start == sizeof(int) && ring_buf->end - ring_buf->buf == 0)
		{
			is_empty = 1;
		}
	}	
	return is_empty;
}

int phudiagfwd_ring_buf2_get_free_size(struct phudiag_ring_buf *ring_buf )
{
	uint8_t* buf_end;
	int free_buf_size = 0;

	if(!ring_buf)
	{
		return -1;
	}
	
	#ifdef PHUDIAG_DEBUG
	#endif
	
	buf_end = ring_buf->buf + ring_buf->buf_size;

	if(ring_buf->start <=  ring_buf->end)
	{
		if(buf_end - ring_buf->end < sizeof(int))
		{
			free_buf_size=  ring_buf->buf_size - (ring_buf->end - ring_buf->start) - (buf_end - ring_buf->end);
		}
		else
		{
			free_buf_size = ring_buf->buf_size - (ring_buf->end - ring_buf->start);
		}
	}
	else
	{
		free_buf_size =  ring_buf->start -ring_buf->end;
	}
	free_buf_size = (free_buf_size  > sizeof(int)) ? (free_buf_size  - sizeof(int)) : 0;
	
	#ifdef PHUDIAG_DEBUG
	#endif
	return free_buf_size;
}



int phudiagfwd_ring_buf2_set_data(struct phudiag_ring_buf *ring_buf ,uint8_t* data,int data_length)
{
	struct phudiag_data *pdata;
	int buffer_size = 0;
	int copyed_size = 0;
	uint8_t* buf_end;

	if(!ring_buf)
	{
		return -1;
	}

	if(!data || 0 == data_length )
	{
		return -1;
	}
	
	#ifdef PHUDIAG_DEBUG
	#endif
	
	buf_end = ring_buf->buf + ring_buf->buf_size;

	
	buffer_size = buf_end - ring_buf->end;
	if(buffer_size >= sizeof(int))
	{
		pdata = (struct phudiag_data *)ring_buf->end;
		pdata->length = data_length;

		buffer_size = buffer_size -sizeof(int);
		if(data_length > buffer_size)
		{	
			if(buffer_size)
			{
				memcpy(pdata->data,data,buffer_size);
			}
			copyed_size = buffer_size;

			data_length -= buffer_size;

			memcpy(ring_buf->buf,data + copyed_size,data_length);
			ring_buf->end = ring_buf->buf + data_length;
		}
		else
		{
			memcpy(pdata->data,data,data_length);
			if(data_length == buffer_size)
			{
				ring_buf->end = ring_buf->buf;
			}
			else
			{
				ring_buf->end = pdata->data + data_length;
			}
		}
	}
	else
	{
		pdata = (struct phudiag_data *)ring_buf->buf;
		pdata->length = data_length;
		memcpy(pdata->data,data,data_length);
		ring_buf->end = ring_buf->buf + data_length;
	}
	
	#ifdef PHUDIAG_DEBUG
	#endif
	
	return data_length;
}

int phudiagfwd_ring_buf2_user_set_data(struct phudiag_ring_buf *ring_buf ,const char __user * data,int data_length)
{
	struct phudiag_data *pdata;
	int ret = 0;
	int buffer_size = 0;
	int copyed_size = 0;
	uint8_t* buf_end;

	if(!ring_buf)
	{
		return -1;
	}

	if(!data || 0 == data_length )
	{
		return -1;
	}

	ret = phudiagfwd_ring_buf2_get_free_size(ring_buf);
	if(ret < 0)
	{
		printk("phudiagchar : phudiagfwd_ring_buf2_user_set_data  ring_buf == NULL \n");
		return -1;
	}

	if(data_length > ret)
	{
		printk("phudiagchar : phudiagfwd_ring_buf2_user_set_data  ring_buf is full ! \n");
		return 0;	
	}
	#ifdef PHUDIAG_DEBUG
	#endif
	
	buf_end = ring_buf->buf + ring_buf->buf_size;
	if(ring_buf->start <= ring_buf->end)
	{
		buffer_size = buf_end - ring_buf->end;	
		if(buffer_size >= sizeof(int))
		{
			pdata = (struct phudiag_data *)ring_buf->end;
			pdata->length = data_length;
	
			buffer_size = buffer_size - sizeof(int);
			if(data_length > buffer_size)
			{	
				if(buffer_size)
				{
					if(copy_from_user((void*)pdata->data,data,buffer_size))
					{
						return -1;
					}
				}
				copyed_size = buffer_size;
				data_length -= buffer_size;
				if(copy_from_user((void*)ring_buf->buf,data + copyed_size,data_length))
				{
					return -1;
				}
				ring_buf->end = ring_buf->buf + data_length;
			}
			else
			{
				if(copy_from_user(pdata->data,data,data_length))
				{
					return -1;
				}
				ring_buf->end = pdata->data + data_length;
			}
		}
		else
		{
			pdata = (struct phudiag_data *)ring_buf->buf;
			pdata->length = data_length;
			if(copy_from_user((void*)pdata->data,data,data_length))
			{
				return -1;
			}
			ring_buf->end = pdata->data + data_length;
		}	
	}
	else
	{
		pdata = (struct phudiag_data *)ring_buf->end;
		pdata->length = data_length;
		
		if(copy_from_user((void*)pdata->data,data,data_length))
		{
			return -1;
		}
		ring_buf->end = pdata->data + data_length;
	}
	
	
	
	#ifdef PHUDIAG_DEBUG
	#endif
	
	return data_length;
}



int phudiagfwd_ring_buf2_get_data(struct phudiag_ring_buf *ring_buf,uint8_t* buf,int buf_size)

{
	struct phudiag_data *pdata;
	int copy_length = 0;
	int data_length = 0;
	uint8_t* buf_end;


	if(!ring_buf)
	{
		return -1;
	}

	if(!buf || 0 == buf_size )
	{
		return -1;
	}
	
	#ifdef PHUDIAG_DEBUG
	#endif
	
	buf_end = ring_buf->buf + ring_buf->buf_size;

	if(buf_end - ring_buf->start >= sizeof(int))
	{
		pdata = (struct phudiag_data *)ring_buf->start;
		data_length = pdata->length;
		if(data_length > buf_size)
		{
			return -2;
		}


		if(buf_end - ring_buf->start >=  data_length + sizeof(int))
		{
			memcpy(buf,pdata->data,data_length);
			ring_buf->start = pdata->data + data_length;
			if(buf_end - ring_buf->start ==  data_length + sizeof(int))
			{
				ring_buf->start = ring_buf->buf;
			}
		}
		else
		{
			copy_length = buf_end - ring_buf->start - sizeof(int);
			if(copy_length)
			{
				memcpy(buf,pdata->data,copy_length);
			}

			memcpy(buf + copy_length ,ring_buf->buf,(data_length - copy_length));
			ring_buf->start = ring_buf->buf + (data_length - copy_length); 
		}
	}

	else
	{
		pdata = (struct phudiag_data *)ring_buf->buf;
		data_length = pdata->length;
		memcpy(buf,pdata->data,data_length);
		ring_buf->start = pdata->data + data_length; 
	}
	
	#ifdef PHUDIAG_DEBUG
	#endif
	
	return data_length;
}


int phudiagfwd_write_data_to_in_buf(uint8_t* buf, int length)
{
	int ret = 0;
	if(0 == buf || length <= 0)
	{
		return -1;
	}
	//mutex_lock(&phudriver->in_buf_mutex);
	ret = phudiagfwd_ring_buf_set_data(phudriver->in_buf,buf,length);
	//mutex_unlock(&phudriver->in_buf_mutex);
	return ret;
}

void phudiagfwd_in_buf_change_busy_state(int smd_avail_data_length,int usb_buf_busy)
{
	int free_size = 0;

	#ifdef PHUDIAG_DEBUG	
	#endif
	
	if(usb_buf_busy)
	{
		phudriver->smd_notify_times++;
		if(phudriver->smd_notify_times <= 5)
		{
			phudriver->in_busy = 1;
			return;
		}
		else
		{
			phudriver->smd_notify_times = 5;
		}
	}
	else
	{
		phudriver->smd_notify_times = 0;
	}

	//mutex_lock(&phudriver->in_buf_mutex);
	if(!phudriver->in_buf_busy)
	{
		phudriver->in_buf_busy = 1;
		free_size = phudiagfwd_ring_buf_get_free_size(phudriver->in_buf);
		phudriver->in_buf_busy = 0;
	}
	else
	{
		phudriver->in_busy = 1;
		return;
	}
	//mutex_unlock(&phudriver->in_buf_mutex);

	if(smd_avail_data_length > free_size)
	{
		phudriver->in_busy = 1;
	}
	else
	{
		phudriver->in_busy = 0;	
	}
}


void phudiagfwd_write_to_smd_work_fn(struct work_struct *work)

{
	int data_length = 0;
	#ifdef PHUDIAG_DEBUG
	int i = 0;
	#endif
	
	#ifdef PHUDIAG_DEBUG
	#endif
	
	if(0 == phudiagfwd_ring_buf2_is_empty(phudriver->out_buf))
	{
		data_length = phudiagfwd_ring_buf2_get_data(phudriver->out_buf,phudriver->smd_buf,PHU_DIAG_SMD_BUF_MAX);	
		if (phudriver->ch && data_length > 0)
		{
			smd_write(phudriver->ch, phudriver->smd_buf,data_length);
			#ifdef PHUDIAG_DEBUG

			for(i=0;i< data_length;i++)
			{
				printk("0x%x,",phudriver->smd_buf[i]);
			}
			printk(" \n");
			#endif
		}
	}
	
	if(0 == phudiagfwd_ring_buf2_is_empty(phudriver->out_buf))
	{
		schedule_work(&(phudriver->phudiag_write_work));
	}
	
}
