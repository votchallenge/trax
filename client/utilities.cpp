#include "utilities.h"


// Based on the example from: http://www.wischik.com/lu/programmer/get-image-size.html
// Retrieve image size from image header
image_size image_get_size(trax_image* image) { 

    image_size result;
    const char* filename = trax_image_get_path(image);

    result.width = -1;
    result.height = -1;

    FILE *f=fopen(filename,"rb");
    if ( f==0 ) return result;

    fseek(f,0,SEEK_END); 
    long len=ftell(f);
    fseek(f,0,SEEK_SET); 

    if (len<24) {fclose(f); return result;}

    // Strategy:
    // reading GIF dimensions requires the first 10 bytes of the file
    // reading PNG dimensions requires the first 24 bytes of the file
    // reading JPEG dimensions requires scanning through jpeg chunks
    // In all formats, the file is at least 24 bytes big, so we'll read that always
    unsigned char buf[24];
    fread(buf,1,24,f);

    // For JPEGs, we need to read the first 12 bytes of each chunk.
    // We'll read those 12 bytes at buf+2...buf+14, i.e. overwriting the existing buf.
    if (buf[0]==0xFF && buf[1]==0xD8 && buf[2]==0xFF && buf[3]==0xE0 && buf[6]=='J' && buf[7]=='F' && buf[8]=='I' && buf[9]=='F') {
        long pos=2;
        while (buf[2]==0xFF) {
            if (buf[3]==0xC0 || buf[3]==0xC1 || buf[3]==0xC2 || buf[3]==0xC3 || buf[3]==0xC9 || buf[3]==0xCA || buf[3]==0xCB) break;
            pos += 2+(buf[4]<<8)+buf[5];
            if (pos+12>len) break;
            fseek(f,pos,SEEK_SET); fread(buf+2,1,12,f);    
        }
    }

    fclose(f);

    // JPEG: (first two bytes of buf are first two bytes of the jpeg file; rest of buf is the DCT frame
    if (buf[0]==0xFF && buf[1]==0xD8 && buf[2]==0xFF) { 
        result.height = (buf[7]<<8) + buf[8];
        result.width = (buf[9]<<8) + buf[10];
    }

    // GIF: first three bytes say "GIF", next three give version number. Then dimensions
    if (buf[0]=='G' && buf[1]=='I' && buf[2]=='F') {
        result.width = buf[6] + (buf[7]<<8);
        result.height = buf[8] + (buf[9]<<8);
    }

    // PNG: the first frame is by definition an IHDR frame, which gives dimensions
    if ( buf[0]==0x89 && buf[1]=='P' && buf[2]=='N' && buf[3]=='G' && buf[4]==0x0D && buf[5]==0x0A && buf[6]==0x1A && buf[7]==0x0A
        && buf[12]=='I' && buf[13]=='H' && buf[14]=='D' && buf[15]=='R') {
        result.width = (buf[16]<<24) + (buf[17]<<16) + (buf[18]<<8) + (buf[19]<<0);
        result.height = (buf[20]<<24) + (buf[21]<<16) + (buf[22]<<8) + (buf[23]<<0);
    }

  return result;
}
