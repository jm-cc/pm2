/*	block.h

	Copyright (C) 2003 Samuel Thibault -- samuel.thibault@fnac.net

*/

/* this is used to enforce the record file structure, with an offset field at
 * the beginning of every block, which tells the offset to jump to to get to
 * the next block */

#define ENTER_BLOCK(fd) \
	{ \
	off_t	__endblock_off; \
	if( read(fd,&__endblock_off,sizeof(__endblock_off)) < (ssize_t) sizeof(__endblock_off) ) \
		{ \
		perror("entering block"); \
		exit(EXIT_FAILURE); \
		} \
	lseek(fd,0,SEEK_CUR);

#define LEAVE_BLOCK(fd) \
	if( lseek(fd,__endblock_off,SEEK_SET) != __endblock_off ) \
		{ \
		perror("seeking forward to end of block"); \
		exit(EXIT_FAILURE); \
		} \
	}	/* matching {} */

#define BEGIN_BLOCK(fd) \
	{ \
	off_t	__block_off,__endblock_off; \
	static const off_t __zero = 0; \
	if( (__block_off=lseek(fd,0,SEEK_CUR)) < 0 ) \
		{ \
		perror("getting block begin seek"); \
		exit(EXIT_FAILURE); \
		} \
	if( write(fd,&__zero,sizeof(__zero)) < 0 ) \
		{ \
		perror("writing dummy offset"); \
		exit(EXIT_FAILURE); \
		} \

#define END_BLOCK(fd)	\
	if( (__endblock_off=lseek(fd,0,SEEK_CUR)) < 0 ) \
		{ \
		perror("getting block end seek"); \
		exit(EXIT_FAILURE); \
		} \
	if( lseek(fd,__block_off,SEEK_SET) != __block_off ) \
		{ \
		perror("seeking back to beginning of block"); \
		exit(EXIT_FAILURE); \
		} \
	if( write(fd,&__endblock_off,sizeof(__endblock_off)) < (ssize_t) sizeof(__endblock_off) ) \
		{ \
		perror("writing end of block offset\n"); \
		exit(EXIT_FAILURE); \
		} \
	LEAVE_BLOCK(fd);

static inline void skip_block( int fd )
	{
	off_t off;
	if( read(fd,&off,sizeof(off)) < sizeof(off) )
		{
		perror("reading end of block offset\n");
		exit(EXIT_FAILURE);
		}
	if( lseek(fd,off,SEEK_SET) != off )
		{
		perror("seeking forward to end of block");
		exit(EXIT_FAILURE);
		}
	}
