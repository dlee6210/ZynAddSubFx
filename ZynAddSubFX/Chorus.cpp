/*
ZynAddSubFX - a software synthesizer

Chorus.C - Chorus and Flange effects
Copyright (C) 2002-2005 Nasca Octavian Paul
Author: Nasca Octavian Paul

This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License 
as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License (version 2) for more details.

You should have received a copy of the GNU General Public License (version 2)
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include <math.h>
#include "Chorus.h"
#include <stdio.h>
#include <stdlib.h>

#define min(a,b)    (((a) < (b)) ? (a) : (b))


Chorus::Chorus(int insertion_,REALTYPE *efxoutl_,REALTYPE *efxoutr_){
	efxoutl=efxoutl_;
	efxoutr=efxoutr_;
	dlk=0;drk=0;
	maxdelay=(int)(MAX_CHORUS_DELAY/1000.0*SAMPLE_RATE);
	delayl=new REALTYPE[maxdelay];
	delayr=new REALTYPE[maxdelay];
	insertion=insertion_;

	filterpars=NULL;
	Ppreset=0;
	setpreset(Ppreset);

	lfo.effectlfoout(&lfol,&lfor);
	dl2=getdelay(lfol);
	dr2=getdelay(lfor);
	cleanup();
};

Chorus::~Chorus(){
	delete [] delayl;
	delete [] delayr;
};

/*
* get the delay value in samples; xlfo is the current lfo value
*/
REALTYPE Chorus::getdelay(REALTYPE xlfo){
	REALTYPE result;
	if (Pflangemode==0){
		result=(delay+xlfo*depth)*SAMPLE_RATE;
	} else result=0;

	//check if it is too big delay(caused bu errornous setdelay() and setdepth()    
	if ((result+0.5)>=maxdelay) {
		//fprintf(stderr,"%s","WARNING: Chorus.C::getdelay(..) too big delay (see setdelay and setdepth funcs.)\n");
		result=maxdelay-1.0;
	};
	return(result);
};

/*
* Apply the effect
*/
void Chorus::out(REALTYPE *smpsl,REALTYPE *smpsr){
	int i;
	dl1=dl2;dr1=dr2;
	lfo.effectlfoout(&lfol,&lfor);

	dl2=getdelay(lfol);
	dr2=getdelay(lfor);

	for (i=0;i<SOUND_BUFFER_SIZE;i++){	
		REALTYPE inl=smpsl[i];
		REALTYPE inr=smpsr[i];
		//LRcross
		REALTYPE l=inl;
		REALTYPE r=inr;
		inl=l*(1.0-lrcross)+r*lrcross;
		inr=r*(1.0-lrcross)+l*lrcross;

		//Left channel

		//compute the delay in samples using linear interpolation between the lfo delays
		mdel=(dl1*(SOUND_BUFFER_SIZE-i)+dl2*i)/SOUND_BUFFER_SIZE;
		if (++dlk>=maxdelay) dlk=0;
		REALTYPE tmp=dlk-mdel+maxdelay*2.0;//where should I get the sample from

		F2I(tmp,dlhi);
		dlhi%=maxdelay;

		dlhi2=(dlhi-1+maxdelay)%maxdelay;
		dllo=1.0-fmodf(tmp,1.0);
		efxoutl[i]=delayl[dlhi2]*dllo+delayl[dlhi]*(1.0-dllo);
		delayl[dlk]=inl+efxoutl[i]*fb;

		//Right channel

		//compute the delay in samples using linear interpolation between the lfo delays
		mdel=(dr1*(SOUND_BUFFER_SIZE-i)+dr2*i)/SOUND_BUFFER_SIZE;
		if (++drk>=maxdelay) drk=0;
		tmp=drk-mdel+maxdelay*2.0;//where should I get the sample from

		F2I(tmp,dlhi);
		dlhi%=maxdelay;	

		dlhi2=(dlhi-1+maxdelay)%maxdelay;
		dllo=1.0-fmodf(tmp,1.0);
		efxoutr[i]=delayr[dlhi2]*dllo+delayr[dlhi]*(1.0-dllo);
		delayr[dlk]=inr+efxoutr[i]*fb;

	};

	if (Poutsub!=0)
		for (i=0;i<SOUND_BUFFER_SIZE;i++){
			efxoutl[i] *= -1.0;
			efxoutr[i] *= -1.0;
		};


	for (int i=0;i<SOUND_BUFFER_SIZE;i++){
		efxoutl[i]*=panning;
		efxoutr[i]*=(1.0-panning);
	};
};

/*
* Cleanup the effect
*/
void Chorus::cleanup(){
	for (int i=0;i<maxdelay;i++){
		delayl[i]=0.0;
		delayr[i]=0.0;
	};

};

/*
* Parameter control
*/
void Chorus::setdepth(unsigned char Pdepth){
	this->Pdepth=Pdepth;
	depth=(pow(8.0,(Pdepth/127.0)*2.0)-1.0)/1000.0;//seconds
};

void Chorus::setdelay(unsigned char Pdelay){
	this->Pdelay=Pdelay;
	delay=(pow(10.0,(Pdelay/127.0)*2.0)-1.0)/1000.0;//seconds
};

void Chorus::setfb(unsigned char Pfb){
	this->Pfb=Pfb;
	fb=(Pfb-64.0)/64.1;
};

void Chorus::setvolume(unsigned char Pvolume){
	this->Pvolume=Pvolume;
	outvolume=Pvolume/127.0;

	if (insertion==0) volume=1.0;
	else volume=outvolume;
};

void Chorus::setpanning(unsigned char Ppanning){
	this->Ppanning=Ppanning;
	panning=Ppanning/127.0;
};

void Chorus::setlrcross(unsigned char Plrcross){
	this->Plrcross=Plrcross;
	lrcross=Plrcross/127.0;
};

void Chorus::setpreset(unsigned char npreset){
	const int PRESET_SIZE=12;
	const int NUM_PRESETS=10;
	unsigned char presets[NUM_PRESETS][PRESET_SIZE]={
		////Chorus1
		//{64,64,50,0,0,90,40,85,64,119,0,0},
		////Chorus2
		//{64,64,45,0,0,98,56,90,64,19,0,0},
		////Chorus3
		//{64,64,29,0,1,42,97,95,90,127,0,0},
		////Celeste1
		//{64,64,26,0,0,42,115,18,90,127,0,0},
		////Celeste2
		//{64,64,29,117,0,50,115,9,31,127,0,1},
		////Flange1
		//{64,64,57,0,0,60,23,3,62,0,0,0},
		////Flange2
		//{64,64,33,34,1,40,35,3,109,0,0,0},
		////Flange3
		//{64,64,53,34,1,94,35,3,54,0,0,1},
		////Flange4
		//{64,64,40,0,1,62,12,19,97,0,0,0},
		////Flange5
		//{64,64,55,105,0,24,39,19,17,0,0,1}};

		//Chorus1
		{64,64,50,0,1,90,40,85,74,119,0,0},
		//Chorus2
		{64,64,45,0,1,98,56,90,84,100,0,0},
		//Chorus3
		{64,64,29,0,1,42,97,95,90,127,0,0},
		//Celeste1
		{64,64,26,0,0,42,115,18,90,127,0,0},
		//Celeste2
		{64,64,29,117,0,50,115,9,31,127,0,1},
		//Flange1
		{64,64,57,0,0,60,23,3,62,0,0,0},
		//Flange2
		{64,64,33,64,1,40,35,3,80,0,0,0},
		//Flange3
		{64,64,53,64,1,94,35,3,54,0,0,1},
		//Flange4
		{64,64,40,0,1,62,12,19,77,0,0,0},
		//Flange5
		{64,64,55,125,0,24,39,19,17,0,0,1}};

		if (npreset>=NUM_PRESETS) npreset=NUM_PRESETS-1;
		for (int n=0;n<PRESET_SIZE;n++) changepar(n,presets[npreset][n]);
		Ppreset=npreset;
};


void Chorus::changepar(int npar,unsigned char value){
	switch(npar){
	case 0:	setvolume(value); /*音量:0-127*/
		break;
	case 1:	setpanning(value); /*平衡0-127*/
		break;
	case 2:	lfo.Pfreq=value;  /*频率0-127*/
		lfo.updateparams();
		break;	
	case 3:	lfo.Prandomness=value; /*随机性0－127*/
		lfo.updateparams();
		break;	
	case 4:	lfo.PLFOtype=value; /*波形类型0:正弦 1：三角2：四边：5：斜边*/
		lfo.updateparams();
		break;	
	case 5:	lfo.Pstereo=value; /*立体声：64－127*/
		lfo.updateparams();
		break;	
	case 6:	setdepth(min(value+20,120)); /*深度0－127value*/
		break;
	case 7:	setdelay(min(value+30,120)); /*延迟0-127value*/
		break;
	case 8:	setfb(min(value+20,110));/*feedback：反馈0-127*/
		break;
	case 9:	setlrcross(min(value+20,120)); /*cross：十字, 交叉,0-127*/
		break;
	case 10:if (value>1) value=1;    /*凸缘模式0-1*/
		Pflangemode=value;
		break;
	case 11:if (value>1) value=1;    /*0-1*/
		Poutsub=value;
		break;
	};
};

unsigned char Chorus::getpar(int npar){
	switch (npar){
	case 0:	return(Pvolume);
		break;
	case 1:	return(Ppanning);
		break;
	case 2:	return(lfo.Pfreq);
		break;
	case 3:	return(lfo.Prandomness);
		break;
	case 4:	return(lfo.PLFOtype);
		break;
	case 5:	return(lfo.Pstereo);
		break;
	case 6:	return(Pdepth);
		break;
	case 7:	return(Pdelay);
		break;
	case 8:	return(Pfb);
		break;
	case 9:	return(Plrcross);
		break;
	case 10:return(Pflangemode);
		break;
	case 11:return(Poutsub);
		break;
	default:return (0);
	};

};




