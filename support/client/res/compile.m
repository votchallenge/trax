
# This script generates tester resources using rescue compiler
# https://github.com/lukacu/rescue/

I = imread('static.jpg');

fh = fopen('static.bin', 'w');
fwrite(fh, I, 'uint8');
fclose(fh);

system('rescue -o ../tester_resources.c -p tester static.jpg static.bin');

delete('static.bin');
