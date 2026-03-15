struct file {
	char	f_flag;
	char	f_count;
	int	f_inode;
	char	*f_offset[2];
} file;

/* flags */
#define	FREAD	01
#define	FWRITE	02
#define	FPIPE	04
