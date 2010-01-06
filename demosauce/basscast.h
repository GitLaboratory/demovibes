#ifndef _H_BASSCAST_
#define _H_BASSCAST_

void BassCastInit();

// keeps reading byte chunks from the input channel
// should block as long no errors appear
void BassCastRun();

#endif