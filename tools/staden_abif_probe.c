/* Optional independent-reader test wrapper. Requires Staden io_lib; not linked into Kleiotide. */
#include <io_lib/Read.h>
#include <io_lib/seqIOABI.h>
#include <stdio.h>
int main(int argc,char**argv){if(argc!=2)return 2;Read*r=read_abi(argv[1]);if(!r){fprintf(stderr,"Staden ABI read failed: %s\n",argv[1]);return 1;}printf("{\"calls\":\"%s\",\"peaks\":[",r->base);for(int i=0;i<r->NBases;i++)printf("%s%u",i?",":"",r->basePos[i]);printf("]");TRACE*data[]={r->traceA,r->traceC,r->traceG,r->traceT};char bases[]="ACGT";for(int c=0;c<4;c++){printf(",\"%c\":[",bases[c]);for(int i=0;i<r->NPoints;i++)printf("%s%u",i?",":"",data[c][i]);printf("]");}printf("}\n");read_deallocate(r);return 0;}
