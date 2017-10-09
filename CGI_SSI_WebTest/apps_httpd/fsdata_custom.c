#include "fs.h"
#include "lwip/def.h"
#include "fsdata.h"

#define FS_ROOT NULL

#define _HTML_INDEX_LEN          1872
#define _HTML_INDEX_ADDR         0x81000
#define _HTML_404_LEN            785
#define _HTML_404_ADDR           0x82000
#define _HTML_LOGO_LEN           4030
#define _HTML_LOGO_ADDR          0x83000
#define _HTML_UPLOAD_LEN         1559
#define _HTML_UPLOAD_ADDR        0x84000
#define _HTML_PNG_LEN            54867
#define _HTML_PNG_ADDR           0x85000

struct file_opt {
	const unsigned char *name;
	const unsigned int  len;
	const unsigned int  addr;
};

static uint32_t custom_file_addr = _HTML_INDEX_ADDR;

const struct file_opt custom_files[] = {
	{"/index.html", _HTML_INDEX_LEN, _HTML_INDEX_ADDR},
	{"/img/header.png", _HTML_PNG_LEN, _HTML_PNG_ADDR},
	{"/img/logo.ico", _HTML_LOGO_LEN, _HTML_LOGO_ADDR},
	{"/kyChu/login.cgi", _HTML_UPLOAD_LEN, _HTML_UPLOAD_ADDR},
	{"/404.html", _HTML_404_LEN, _HTML_404_ADDR}
};

#define NUM_CUSTOM_FILES (sizeof(custom_files) / sizeof(struct file_opt))

int fs_open_custom(struct fs_file *file, const char *name) {
	int i = NUM_CUSTOM_FILES - 1;
//	printf("exp file: %s, files = %d.\n", name, NUM_CUSTOM_FILES);
	for(; i >= 0; -- i) {
		if(!strcmp(name, custom_files[i].name)) {
//			printf("find exp file: %s.\n", custom_files[i].name);
			file->index = 0;
			file->len = custom_files[i].len;
			file->flags = 1;
			file->pextension = NULL;
			custom_file_addr = custom_files[i].addr;
			return 1;
		}
//		printf("find file: %s.\n", custom_files[i]);
	}
	return 0;
}

int fs_read_custom(struct fs_file *file, char *buffer, int count)
{
	uint32_t read;
	uint32_t len;

//	printf("read custom data.\n");

	if (file->is_custom_file) {
	  read = file->len - file->index;
	  if(read > count) {
	    read = count;
	  }

	  spi_flash_read(custom_file_addr + file->index, (uint32_t)buffer, read);
	  file->index += read;
	  return(read);
	}
	return FS_READ_EOF;
}

void fs_close_custom(struct fs_file *file)
{
//	printf("close custom file.\n");
	file->index = 0;
	file->len = 0;
	custom_file_addr = _HTML_INDEX_ADDR;
}
