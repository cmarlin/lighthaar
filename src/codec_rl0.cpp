#include "codec_rl0.h"

#include <string.h>


int RL0count(const uint8_t * const buf, int pos, int len)
{
	int count=0;
	int c0=0;
	while( pos<len && count<128)
	{
		count++;
		if (buf[pos]!=0)
			c0=0;
		else 
		{
			c0++;
			if (c0==2)
				if (count==c0)
				{
					pos++;
					while( pos<len && c0<129)
					{
						if (buf[pos]==0)
							c0++;
						else return -c0;
						pos++;
					}
					return -c0;
				}
				else 
				{
					count-=c0;
					return count;
				}
		}
		pos++;

	}
	return count;
}

void toRL0(uint8_t* rl0, int* rl0size, const uint8_t* const buf, const int size)
{
	if(buf == NULL)
	{
		*rl0size = size+size/128+1;
		return;
	}

	int n=0,p=0,count;
	//unsigned char *rl0;
	//rl0=new unsigned char[size+size/128+1];

	while( p<size )
	{
		count=RL0count(buf,p,size);
		if (count>0)
		{
			rl0[n++]=count-1;
			memcpy(&rl0[n],&buf[p],count);
			n+=count;
			p+=count;
		}
		else
			if (count<0)
			{
				rl0[n++]=count+1;
				p+=-count;
			}
	}

	*rl0size=n;
}

void fromRL0(uint8_t *out, const uint8_t * const rl0, const int rl0size)
{
	int p=0,n=0;
	int count;

	while( p<rl0size )
	{
		count=(signed char)rl0[p];
		if (count<0)
		{
			memset(&out[n],0,-count+1);
			n-=count-1;
			p++;
		}
		else
		{
			memcpy(&out[n],&rl0[p+1],count+1);
			p+=count+2;
			n+=count+1;
		}
	}
}

