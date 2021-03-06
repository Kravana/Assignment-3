#include "pixutils.h"

//private methods -> make static
static pixMap* pixMap_init(unsigned char arrayType);
static pixMap* pixMap_copy(pixMap *p);


static pixMap* pixMap_init(unsigned char arrayType){
	//initialize everything to zero except arrayType
	pixMap* pixM = malloc(sizeof(pixMap));
	pixM->image = 0;
	pixM->imageHeight = 0;
	pixM->imageWidth = 0;
	pixM->pixArray_arrays = 0;
	pixM->pixArray_blocks = 0;
	pixM->pixArray_overlay = 0;
	pixM->arrayType = arrayType;
	return pixM;
}	

void pixMap_destroy (pixMap **p){
 //free all mallocs and put a zero pointer in *p
	if ((*p)->arrayType == 1) {
		for (int i = 0; i < (*p)->imageHeight; i++) {
			free((*p)->pixArray_blocks[i]);
		}
	}
	free((*p)->pixArray_arrays);
	free((*p)->pixArray_blocks);
	free((*p)->pixArray_overlay);
	free((*p)->image);
	free(*p);
}
	
pixMap *pixMap_read(char *filename,unsigned char arrayType){
	//library call reads in the image into p->image and sets the width and height
	pixMap *p=pixMap_init(arrayType);
	int error;
	if((error=lodepng_decode32_file(&(p->image), &(p->imageWidth), &(p->imageHeight),filename))){
	fprintf(stderr,"error %u: %s\n", error, lodepng_error_text(error));
		return 0;
	}
	//allocate the 2-D rgba arrays
	
	if (arrayType ==0){
		//can only allocate for the number of rows - each row will be an array of MAXWIDTH
		//copy each row of the image into each row
		/*
		 * Allocate p->pixArray_arrays a number of bytes equal to sizeof(p->pixArray_arrays) * p->imageHeight.
		 * Then, for each row (rgba[MAXWIDTH], position 0 <= i < p->imageHeight), 
		 * initialize rgba structs from every 4 chars in p->image.
		 */
		if (p->imageWidth > MAXWIDTH) {
			fprintf(stderr,"The image width is greater than MAXWIDTH");
			return 0;
		}
		p->pixArray_arrays = malloc(sizeof(rgba[MAXWIDTH]) * p->imageHeight);
		for (int i = 0; i < p->imageHeight; i++) {
			for (int j = 0; j < p->imageWidth; j++) {
				int rowPos = i * p->imageWidth * 4;
				int columnPos = j * 4;
				rgba newPix = {p->image[rowPos + columnPos + 0],
					p->image[rowPos + columnPos + 1],
					p->image[rowPos + columnPos + 2],
					p->image[rowPos + columnPos + 3]};
				p->pixArray_arrays[i][j] = newPix;
			}
		}
	}	
	else if (arrayType ==1){
		//allocate a block of memory (dynamic array of p->imageHeight) to store the pointers
		//use a loop allocate a block of memory for each row
		//copy each row of the image into the newly allocated block
		/*
		 * Allocate p->pixArray-blocks a number of bytes equal to sizeof(rgba*) * p->imageHeight.
		 * Then, for each row (rgba*, position 0 <= i < p->imageHeight), 
		 * allocate a number of bytes equal to sizeof(rgba) * p->imageWidth and copy
		 * rgba data into an rgba struct and add it the row p->imageWidth times.
		 */
		p->pixArray_blocks = malloc(sizeof(rgba*) * p->imageHeight);
		for (int i = 0; i < p->imageHeight; i++) {
			p->pixArray_blocks[i] = malloc(sizeof(rgba) * p->imageWidth);
			for (int j = 0; j < p->imageWidth; j++) {
				int rowPos = i * p->imageWidth * 4;
				int columnPos = j * 4;
				rgba newPix = {p->image[rowPos + columnPos + 0],
					p->image[rowPos + columnPos + 1],
					p->image[rowPos + columnPos + 2],
					p->image[rowPos + columnPos + 3]};
				p->pixArray_blocks[i][j] = newPix;
			}
		}
	}
	else if (arrayType ==2){
		//allocate a block of memory (dynamic array of p->imageHeight) to store the pointers
		//set the first pointer to the start of p->image
		//each subsequent pointer is the previous pointer + p->imageWidth
		p->pixArray_overlay = malloc(sizeof(rgba*) * p->imageHeight);
		p->pixArray_overlay[0] = (rgba*) p->image;
		for (int i = 1; i < p->imageHeight; i++) {
			p->pixArray_overlay[i] = p->pixArray_overlay[i - 1] + p->imageWidth;
		}
	}
	else{
		return 0;
	}				
	return p;
}
int pixMap_write(pixMap *p,char *filename){
	int error=0;
	//for arrayType 1 and arrayType 2 have to write out a controws  to the image using memcpy
	if (p->arrayType ==0){
		//have to copy each row of the array into the corresponding row of the image
		for (int i = 0; i < p->imageHeight; i++) {
			memcpy(p->image + i * p->imageWidth * 4, 
			p->pixArray_arrays + i, 
			p->imageWidth * sizeof(rgba));
		}
	}	
	else if (p->arrayType ==1){
		//have to copy each row of the array into the corresponding row of the image
		for (int i = 0; i < p->imageHeight; i++) {
			memcpy(p->image + i * p->imageWidth * 4, 
			p->pixArray_blocks[i], 
			p->imageWidth * sizeof(rgba));
		}
	
	}
	//library call to write the image out 
 if(lodepng_encode32_file(filename, p->image, p->imageWidth, p->imageHeight)){
  fprintf(stderr,"error %u: %s\n", error, lodepng_error_text(error));
  return 1;
	}
	return 0;
}	 

int pixMap_rotate(pixMap *p,float theta){
	pixMap *oldPixMap=pixMap_copy(p);
	if(!oldPixMap)return 1;
	const float ox=p->imageWidth/2.0f;
	const float oy=p->imageHeight/2.0f;
	const float s=sin(degreesToRadians(-theta));
	const float c=cos(degreesToRadians(-theta));
	for(int y=0;y<p->imageHeight;y++){
		for(int x=0;x<p->imageWidth;x++){ 
			float rotx = c*(x-ox) - s * (oy-y) + ox;
			float roty = -(s*(x-ox) + c * (oy-y) - oy);
			int rotj=rotx+.5;
			int roti=roty+.5; 
			if(roti >=0 && roti < oldPixMap->imageHeight && rotj >=0 && rotj < oldPixMap->imageWidth){
				if(p->arrayType==0) memcpy(p->pixArray_arrays[y]+x,oldPixMap->pixArray_arrays[roti]+rotj,sizeof(rgba));
				else if(p->arrayType==1) memcpy(p->pixArray_blocks[y]+x,oldPixMap->pixArray_blocks[roti]+rotj,sizeof(rgba));
				else if(p->arrayType==2) memcpy(p->pixArray_overlay[y]+x,oldPixMap->pixArray_overlay[roti]+rotj,sizeof(rgba));			 
			}
			else{
				if(p->arrayType==0) memset(p->pixArray_arrays[y]+x,0,sizeof(rgba));
				else if(p->arrayType==1) memset(p->pixArray_blocks[y]+x,0,sizeof(rgba));
				else if(p->arrayType==2) memset(p->pixArray_overlay[y]+x,0,sizeof(rgba));		
			}		
		}	
	}
	pixMap_destroy(&oldPixMap);
	return 0;
}

pixMap *pixMap_copy(pixMap *p){
	pixMap *new=pixMap_init(p->arrayType);
	//allocate memory for new image of the same size a p->image and copy the image
	new->imageHeight = p->imageHeight;
	new->imageWidth = p->imageWidth;
	new->image = malloc(p->imageWidth * p->imageHeight * sizeof(rgba));
	memcpy(new->image, p->image, new->imageWidth * new->imageHeight * sizeof(rgba));
	
	//allocate memory and copy the arrays. 
	if (new->arrayType ==0){
		//insert code
		new->pixArray_arrays = malloc(sizeof(rgba[MAXWIDTH]) * p->imageHeight);
		memcpy(new->pixArray_arrays, 
		p->pixArray_arrays, 
		sizeof(rgba[MAXWIDTH]) * p->imageHeight);
		
	}	
	else if (new->arrayType ==1){
		//insert code
		new->pixArray_blocks = malloc(sizeof(rgba*) * new->imageHeight);
		for (int i = 0; i < new->imageHeight; i++) {
			new->pixArray_blocks[i] = malloc(sizeof(rgba) * new->imageWidth);
			memcpy(new->pixArray_blocks[i], 
			p->pixArray_blocks[i], 
			sizeof(rgba) * new->imageWidth);
		}
	}
	else if (new->arrayType ==2){
		//insert code
		new->pixArray_overlay = malloc(sizeof(rgba*) * new->imageHeight);
		new->pixArray_overlay[0] = (rgba*) new->image;
		for (int i = 1; i < new->imageHeight; i++) {
			new->pixArray_overlay[i] = new->pixArray_overlay[i - 1] + p->imageWidth;
		}
	}
	return new;
} 
