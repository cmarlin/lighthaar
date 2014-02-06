//
// Arithmetic encoder/decoder
//

#include <stdint.h>


struct BinStream
{
	uint8_t*	m_pBuffer;
	intptr_t	m_iBufferSize;
	intptr_t	m_iPosition;

	void		Open(void* pBuffer);
	//void		Close();
	int			Read(void* pBuffer, int iBufferSize);
	int			Write(void* pBuffer, int iBufferSize);
	intptr_t	Seek(intptr_t iPosition, int iOrigin);

	void		SetBufferSize(intptr_t iBufferSize);
	intptr_t	GetBufferSize() const;
};

#define MAXIMUM_SCALE   16383  /* Maximum allowed frequency count */
#define ESCAPE          256    /* The escape symbol               */
#define DONE            -1     /* The output stream empty  symbol */
#define FLUSH           -2     /* The symbol to flush the model   */

/*
 * A symbol can either be represented as an int, or as a pair of
 * counts on a scale.  This structure gives a standard way of
 * defining it as a pair of counts.
 */
typedef struct {
                unsigned short int low_count;
                unsigned short int high_count;
                unsigned short int scale;
               } SYMBOL;

class ArithEncoder
{
public:

int BUFFER_SIZE,max_read,num_read;

unsigned short int code;  /* The present input code value       */
unsigned short int low;   /* Start of the current code range    */
unsigned short int high;  /* End of the current code range      */
long underflow_bits;      /* Number of underflow bits pending   */


short int storage[ 258 ];
short int *totals;
char *buffer;					/* This is the i/o buffer    */
char *current_byte;             /* Pointer to current byte   */


int output_mask;                /* During output, this byte  */
                                       /* contains the mask that is */
                                       /* applied to the output byte*/
                                       /* if the output bit is a 1  */
int input_bytes_left;           /* During input, these three */
int input_bits_left;            /* variables keep track of my*/
int past_eof;                   /* input state.  The past_eof*/
                                       /* byte comes about because  */
                                       /* of the fact that there is */
                                       /* a possibility the decoder */
                                       /* can legitimately ask for  */
                                       /* more bits even after the  */
                                       /* entire file has been      */
                                       /* sucked dry.               */
ArithEncoder(int bufsize)
{
	BUFFER_SIZE=bufsize;
	buffer=new char[bufsize];
	totals = storage + 1;
}

~ArithEncoder()
{
	if (BUFFER_SIZE>0)
		delete buffer;
}
void write(BinStream *stream, unsigned char *buf, int size);

void initialize_arithmetic_encoder();
void encode_symbol( BinStream *stream, SYMBOL *s );
void flush_arithmetic_encoder( BinStream *stream );
short int get_current_count( SYMBOL *s );
void initialize_arithmetic_decoder( BinStream *stream,int max);
void remove_symbol_from_stream( BinStream *stream, SYMBOL *s);

void initialize_model(void);
void update_model( int symbol );
int convert_int_to_symbol( int c, SYMBOL *s );
void get_symbol_scale( SYMBOL *s );
int convert_symbol_to_int( int count, SYMBOL *s);

void initialize_output_bitstream(void);
void output_bit( BinStream *stream, int bit );
void flush_output_bitstream( BinStream *stream );
void initialize_input_bitstream();
short int input_bit( BinStream *stream );
};
