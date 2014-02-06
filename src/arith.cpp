//#include "stdafx.h"
#include <stdio.h>
#include <assert.h>
#include "Arith.h"

void ArithEncoder::write(BinStream *stream, unsigned char *buf, int size)
{
	int n;
    SYMBOL s;
    for ( n=0;n<size;n++ )
    {
		convert_int_to_symbol( buf[n], &s );
		encode_symbol( stream, &s );
		update_model( buf[n] );
    }
}

/*
 * This routine must be called to initialize the encoding process.
 * The high register is initialized to all 1s, and it is assumed that
 * it has an infinite string of 1s to be shifted into the lower bit
 * positions when needed.
 */
void ArithEncoder::initialize_arithmetic_encoder()
{
    low = 0;
    high = 0xffff;
    underflow_bits = 0;
}

/*
 * This routine is called to encode a symbol.  The symbol is passed
 * in the SYMBOL structure as a low count, a high count, and a range,
 * instead of the more conventional probability ranges.  The encoding
 * process takes two steps.  First, the values of high and low are
 * updated to take into account the range restriction created by the
 * new symbol.  Then, as many bits as possible are shifted out to
 * the output stream.  Finally, high and low are stable again and
 * the routine returns.
 */
void ArithEncoder::encode_symbol( BinStream *stream, SYMBOL *s )
{
    long range;
/*
 * These three lines rescale high and low for the new symbol.
 */
    range = (long) ( high-low ) + 1;
    high = low + (unsigned short int )
                 (( range * s->high_count ) / s->scale - 1 );
    low = low + (unsigned short int )
                 (( range * s->low_count ) / s->scale );
/*
 * This loop turns out new bits until high and low are far enough
 * apart to have stabilized.
 */
    for ( ; ; )
    {
/*
 * If this test passes, it means that the MSDigits match, and can
 * be sent to the output stream.
 */
        if ( ( high & 0x8000 ) == ( low & 0x8000 ) )
        {
            output_bit( stream, high & 0x8000 );
            while ( underflow_bits > 0 )
            {
                output_bit( stream, ~high & 0x8000 );
                underflow_bits--;
            }
        }
/*
 * If this test passes, the numbers are in danger of underflow, because
 * the MSDigits don't match, and the 2nd digits are just one apart.
 */
        else if ( ( low & 0x4000 ) && !( high & 0x4000 ))
        {
            underflow_bits += 1;
            low &= 0x3fff;
            high |= 0x4000;
        }
        else
            return ;
        low <<= 1;
        high <<= 1;
        high |= 1;
    }
}

/*
 * At the end of the encoding process, there are still significant
 * bits left in the high and low registers.  We output two bits,
 * plus as many underflow bits as are necessary.
 */
void ArithEncoder::flush_arithmetic_encoder( BinStream *stream )
{
    output_bit( stream, low & 0x4000 );
    underflow_bits++;
    while ( underflow_bits-- > 0 )
        output_bit( stream, ~low & 0x4000 );
}

/*
 * When decoding, this routine is called to figure out which symbol
 * is presently waiting to be decoded.  This routine expects to get
 * the current model scale in the s->scale parameter, and it returns
 * a count that corresponds to the present floating point code:
 *
 *  code = count / s->scale
 */
short int ArithEncoder::get_current_count( SYMBOL *s )
{
    long range;
    short int count;

    range = (long) ( high - low ) + 1;
    count = (short int)
            ((((long) ( code - low ) + 1 ) * s->scale-1 ) / range );
    return( count );
}

/*
 * This routine is called to initialize the state of the arithmetic
 * decoder.  This involves initializing the high and low registers
 * to their conventional starting values, plus reading the first
 * 16 bits from the input stream into the code value.
 */
void ArithEncoder::initialize_arithmetic_decoder( BinStream *stream ,int max)
{
    int i;

	max_read=max+1;
	num_read=0;
    code = 0;
    for ( i = 0 ; i < 16 ; i++ )
    {
        code <<= 1;
        code += input_bit( stream );
    }
    low = 0;
    high = 0xffff;
}

/*
 * Just figuring out what the present symbol is doesn't remove
 * it from the input bit stream.  After the character has been
 * decoded, this routine has to be called to remove it from the
 * input stream.
 */
void ArithEncoder::remove_symbol_from_stream( BinStream *stream, SYMBOL *s)
{
    long range;

/*
 * First, the range is expanded to account for the symbol removal.
 */
    range = (long)( high - low ) + 1;
    high = low + (unsigned short int)
                 (( range * s->high_count ) / s->scale - 1 );
    low = low + (unsigned short int)
                 (( range * s->low_count ) / s->scale );
/*
 * Next, any possible bits are shipped out.
 */
    for ( ; ; )
    {
/*
 * If the MSDigits match, the bits will be shifted out.
 */
        if ( ( high & 0x8000 ) == ( low & 0x8000 ) )
        {
        }
/*
 * Else, if underflow is threatining, shift out the 2nd MSDigit.
 */
        else if ((low & 0x4000) == 0x4000  && (high & 0x4000) == 0 )
        {
            code ^= 0x4000;
            low   &= 0x3fff;
            high  |= 0x4000;
        }
/*
 * Otherwise, nothing can be shifted out, so I return.
 */
        else
            return;
        low <<= 1;
        high <<= 1;
        high |= 1;
        code <<= 1;

       	code += input_bit( stream );
	
    }
}

/*
 * When the model is first started up, each symbols has a count of
 * 1, which means a low value of c+1, and a high value of c+2.
 */
void ArithEncoder::initialize_model(void)
{
    short int i;

    for ( i = -1 ; i <= 256 ; i++ )
        totals[ i ] = i + 1;
}

/*
 * Updating the model means incrementing every single count from
 * the high value for the symbol on up to the total.  Then, there
 * is a complication.  If the cumulative total has gone up to
 * the maximum value, we need to rescale.  Fortunately, the rescale
 * operation is relatively rare.
 */
void ArithEncoder::update_model( int symbol )
{
    int i;

    for ( symbol++ ; symbol <= 256; symbol++ )
        totals[ symbol ]++;
    if ( totals[ 256 ] == MAXIMUM_SCALE )
    {
        for ( i = 0 ; i <= 256 ; i++ )
	{
            totals[ i ] /= 2;
            if ( totals[ i ] <= totals[ i-1 ] )
                totals[ i ] = totals[ i-1 ] + 1;
	}
    }
}

/*
 * Finding the low count, high count, and scale for a symbol
 * is really easy, because of the way the totals are stored.
 * This is the one redeeming feature of the data structure used
 * in this implementation.  Note that this routine returns an
 * int, but it is not used in this routine.  The return value
 * from convert_int_to_symbol is used in model-2.c.
 */
int ArithEncoder::convert_int_to_symbol( int c, SYMBOL *s )
{
    s->scale = totals[ 256 ];
    s->low_count = totals[ c ];
    s->high_count = totals[ c+1 ];
    return( 0 );
}

/*
 * Getting the scale for the current context is easy.
 */
void ArithEncoder::get_symbol_scale( SYMBOL *s )
{
    s->scale = totals[ 256 ];
}

/*
 * During decompression, we have to search through the table until
 * we find the symbol that straddles the "count" parameter.  When
 * it is found, it is returned. The reason for also setting the
 * high count and low count is so that symbol can be properly removed
 * from the arithmetic coded input.
 */
int ArithEncoder::convert_symbol_to_int( int count, SYMBOL *s)
{
    int c;

    for ( c = 255; count < totals[ c ] ; c-- )
	;
    s->high_count = totals[ c+1 ];
    s->low_count = totals[ c ];
    return( c );
}

/*
 * This routine is called once to initialze the output bitstream.
 * All it has to do is set up the current_byte pointer, clear out
 * all the bits in my current output byte, and set the output mask
 * so it will set the proper bit next time a bit is output.
 */
void ArithEncoder::initialize_output_bitstream(void)
{
    current_byte = buffer;
    *current_byte = 0;
    output_mask = 0x80;
}

/*
 * The output bit routine just has to set a bit in the current byte
 * if requested to.  After that, it updates the mask.  If the mask
 * shows that the current byte is filled up, it is time to go to the
 * next character in the buffer.  If the next character is past the
 * end of the buffer, it is time to flush the buffer.
 */
void ArithEncoder::output_bit( BinStream *stream, int bit )
{
    if ( bit )
        *current_byte |= output_mask;
    output_mask >>= 1;
    if ( output_mask == 0 )
    {
        output_mask = 0x80;
        current_byte++;
        if ( current_byte == ( buffer + BUFFER_SIZE ) )
        {
            stream->Write(buffer, BUFFER_SIZE);  
            current_byte = buffer;
        }
        *current_byte = 0;
    }
}

/*
 * When the encoding is done, there will still be a lot of bits and
 * bytes sitting in the buffer waiting to be sent out.  This routine
 * is called to clean things up at that point.
 */
void ArithEncoder::flush_output_bitstream( BinStream *stream )
{
	stream->Write(buffer,((int)(current_byte - buffer )) + 1);  

    current_byte = buffer;
}

/*
 * Bit oriented input is set up so that the next time the input_bit
 * routine is called, it will trigger the read of a new block.  That
 * is why input_bits_left is set to 0.
 */
void ArithEncoder::initialize_input_bitstream()
{
    input_bits_left = 0;
    input_bytes_left = 1;
    past_eof = 0;
}

/*
 * This routine reads bits in from a file.  The bits are all sitting
 * in a buffer, and this code pulls them out, one at a time.  When the
 * buffer has been emptied, that triggers a new file read, and the
 * pointers are reset.  This routine is set up to allow for two dummy
 * bytes to be read in after the end of file is reached.  This is because
 * we have to keep feeding bits into the pipeline to be decoded so that
 * the old stuff that is 16 bits upstream can be pushed out.
 */
short int ArithEncoder::input_bit( BinStream *stream )
{
    if ( input_bits_left == 0 )
    {
		num_read++;
        current_byte++;
        input_bytes_left--;
        input_bits_left = 8;
        if ( input_bytes_left == 0)
        {
			if (num_read>max_read)
				return 0;
			input_bytes_left=stream->Read(buffer, BUFFER_SIZE);
            if ( input_bytes_left == 0 )
				return 0;
            current_byte = buffer;
        }
    }
    input_bits_left--;
    return ( ( *current_byte >> input_bits_left ) & 1 );
}

void BinStream::Open(void* pBuffer)
{
	m_iBufferSize = INT_MAX;
	m_pBuffer = (uint8_t*)pBuffer;
	m_iPosition = 0;
}
/*
void BinStream::Close()
{
	if(!m_bRead)
	{
		int iWriteSize = m_pPosition - m_pBuffer;
		assert(iWriteSize<=m_iBufferSize);
		uint8_t* pSize = m_pBuffer-4;
		pSize[0] = iWriteSize&0xff; pSize[1] = (iWriteSize>>8)&0xff; pSize[2] = (iWriteSize>>16)&0xff; pSize[3] = iWriteSize>>24;
	}
}
*/
int BinStream::Read(void* pBuffer, int iBufferSize)
{
	if(iBufferSize<=0) return 0;

	int iReadSize = iBufferSize;
	if(m_iBufferSize!=INT_MAX)
	{
		iReadSize = (m_iPosition+iBufferSize < m_iBufferSize) ?
		iBufferSize
		: m_iBufferSize-m_iPosition;
	}
	memcpy(pBuffer, m_pBuffer+m_iPosition, iReadSize);
	m_iPosition += iReadSize;
	return iReadSize;
}

int	BinStream::Write(void* pBuffer, int iBufferSize)
{
	if(iBufferSize<=0) return 0;

	int iWriteSize = iBufferSize;
	if(m_iBufferSize!=INT_MAX)
	{
		iWriteSize = m_iPosition+iBufferSize < m_iBufferSize ?
		iBufferSize
		: m_iBufferSize-m_iPosition;
	}
	memcpy(m_pBuffer+m_iPosition, pBuffer, iWriteSize);
	m_iPosition += iWriteSize;
	return iWriteSize;
}

intptr_t BinStream::Seek(intptr_t iPosition, int iOrigin)
{
	switch(iOrigin)
	{
	case SEEK_SET:	m_iPosition = iPosition;				break;
	case SEEK_CUR:	m_iPosition += iPosition;				break;
	case SEEK_END:
		{
			if(m_iBufferSize!=INT_MAX
				&& iPosition<=0
				&& (-iPosition)<=m_iBufferSize)
			{
				m_iPosition = m_iBufferSize - iPosition;
			}
		}
		break;

	default: assert(false);
	}

	return m_iPosition;
}

void BinStream::SetBufferSize(intptr_t iBufferSize)
{
	m_iBufferSize = iBufferSize;
}

intptr_t BinStream::GetBufferSize() const
{
	return m_iBufferSize;
}

