// ofxFlite~ -  8bit flite2 synth voice - (c) s373.net/x 2016
// adapted from s373AVSpeak~, in ofxs373A~, 2012+
// Created by andré sier on 20160530.

/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 1999                             */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  December 1999                                    */
/*************************************************************************/
/*                                                                       */
/*  Light weight run-time speech synthesis system, public API            */
/*                                                                       */
/*************************************************************************/


#pragma once
#include <string>
#include <vector>
#include "Poco/Mutex.h"
#include "ofMain.h"
#include "flite.h"


		#ifdef __cplusplus
		extern "C"{
		#endif

			cst_voice* register_cmu_us_kal();
			cst_voice* register_cmu_us_kal16();
			cst_voice* register_cmu_us_awb();
			cst_voice* register_cmu_us_rms();
			cst_voice* register_cmu_us_slt();

			void  unregister_cmu_us_kal();
			void  unregister_cmu_us_kal16();
			void  unregister_cmu_us_awb();
			void  unregister_cmu_us_rms();
			void  unregister_cmu_us_slt();

			void usenglish_init(cst_voice *v);
			cst_lexicon *cmulex_init(void);

			void cmu_indic_lang_init(cst_voice *v);
			cst_lexicon *cmu_indic_lex_init(void);

		#ifdef __cplusplus
		}
		#endif




class s373AVSpeakT : public ofThread{
protected:
	std::string		systemcall;
	cst_voice 		*flitevoice;
	int				samplerate,buffersize,flitesamplerate,
					flitevoiceom,
 					numbuffersamples,
	 				runningnumsamples,
					bufferhead, maxbufferhead,
					oldfullbuffersize;
	float 			bufferlocf, bufferspeedf,
					bufferlocminpercent, bufferlocmaxpercent,
					bufferdistpercent;
	std::string 	fullbufferstr,bufferstr;
	bool 			loop, mayread, reachedend;
	vector<std::string>	fullbuffers;
public:
	void setup(int sr, int bs, std::string scall,
		float speed, bool doloop = true, int vom = 0
		){

		samplerate=sr;
		buffersize=bs;

		setLoop(doloop);

		flite_init();

		flite_add_lang("eng",usenglish_init,cmulex_init);
		flite_add_lang("cmu_indic_lang",cmu_indic_lang_init,cmu_indic_lex_init);

		flitevoiceom = vom;

		switch(flitevoiceom){
			default: case 0: flitevoice = register_cmu_us_kal(); break;
			case 1: flitevoice = register_cmu_us_kal16(); break;
			case 2: flitevoice = register_cmu_us_awb(); break;
			case 3: flitevoice = register_cmu_us_rms(); break;
			case 4: flitevoice = register_cmu_us_slt(); break;
		}

		cmu_indic_lang_init(flitevoice);

		systemcall = scall;
		numbuffersamples=bs;
		runningnumsamples = 0;

		fullbufferstr = "";
		bufferstr = "";

		bufferhead=0;
		maxbufferhead = 1;
		oldfullbuffersize = numbuffersamples;

		bufferlocf = 0.0f;
		bufferspeedf = speed;
		flitesamplerate = 16000; ///!
		// flitesamplerate = 8000; ///!
		setSpeakSpeed(speed);
		setMinMaxBufferLoc(0,1);

		for(int i=0; i<numbuffersamples;i++){
			bufferstr+='\0';
			fullbufferstr+='\0';
		}


		setSystemCall(scall);
	}







	void setNewVoice(int nvoice){

		switch(flitevoiceom){
			default: case 0: unregister_cmu_us_kal(); break;
			case 1: unregister_cmu_us_kal16(); break;
			case 2: unregister_cmu_us_awb(); break;
			case 3: unregister_cmu_us_rms(); break;
			case 4: unregister_cmu_us_slt(); break;
		}

		flitevoiceom = nvoice;

		switch(flitevoiceom){
			default: case 0: flitevoice = register_cmu_us_kal(); break;
			case 1: flitevoice = register_cmu_us_kal16(); break;
			case 2: flitevoice = register_cmu_us_awb(); break;
			case 3: flitevoice = register_cmu_us_rms(); break;
			case 4: flitevoice = register_cmu_us_slt(); break;
		}



	}


	const std::string  loadFile(const std::string & fn){

		while (isThreadRunning()) {
			cout << "thread running and trying load file " << fn << endl;
			ofSleepMillis(100);
		}


		ifstream myfile;
		myfile.open (ofToDataPath(fn).c_str());
		if(!myfile){
			cout << "error opening "<< fn << endl;
			return "false";
		}

		cout << this << " opening "<< fn << endl;

		std::string line="";
		std::string fulltext="";

		int nlines=0;
		while(std::getline(myfile, line)){
			fulltext += line;
			nlines++;
		}

		cout << this << " speakloadfile nlines " << nlines << " nchars "<< fulltext.size() << endl;
		setSystemCall(fulltext);

		return fulltext;
	}



	void setSystemCall(const std::string & call){
		if(isThreadRunning())stopThread();
		systemcall = call;
		ofSleepMillis(25);
		if(isThreadRunning())stopThread();
		if(!isThreadRunning()) startThread();
	}

	void setSpeakSpeed(float s){
		// 8000 / 44100
		bufferspeedf = (s *  (flitesamplerate/(float)samplerate));
	}

	float getBufferLocPercent(){
		return bufferlocf / (float)oldfullbuffersize;
	}

	void setBufferLocPercent(float per){
		bufferlocf = per * (float)oldfullbuffersize;
	}

	void setLoop(bool l){
		loop = l;
		mayread = true;
		reachedend = false;
	}
	bool getLoop(){ return loop; }

	void setMinMaxBufferLoc(float min, float max){
		min = CLAMP(min,0,1);
		max = CLAMP(max,0,1);
		bufferlocminpercent = min;
		bufferlocmaxpercent = max;
		bufferdistpercent = bufferlocmaxpercent - bufferlocminpercent;
	}

	const std::string & readStr(int numsamptstoread){
		if(isThreadRunning()){
			return bufferstr;
		}

		if(oldfullbuffersize!=fullbufferstr.size()){
			oldfullbuffersize = fullbufferstr.size();
			maxbufferhead = oldfullbuffersize / numbuffersamples;
		}

		int maxlen = oldfullbuffersize-1;
		int startsamp = bufferlocminpercent * maxlen;
		int endsamp = bufferlocmaxpercent * maxlen;
		int distsamp = bufferdistpercent * maxlen;

		if(loop || mayread){

			for(int i=0; i<numbuffersamples;i++){

				bufferlocf += bufferspeedf;

				// if(bufferlocf>=maxlen){
				if(bufferlocf>=endsamp){
					bufferlocf-=distsamp;
					reachedend = true;
					mayread = false;
				} else if(bufferlocf<startsamp){
					bufferlocf+=distsamp;
					reachedend = true;
					mayread = false;
				}



				int idx = (int) bufferlocf;
				bufferstr[i] = fullbufferstr[idx];

			}


		}

		return bufferstr;

	}

	const std::string & readBufferN(int nbuffer){
		if(isThreadRunning()){
			return bufferstr;
		}

		if(oldfullbuffersize!=fullbufferstr.size()){
			oldfullbuffersize = fullbufferstr.size();
			maxbufferhead = oldfullbuffersize / numbuffersamples;
		}

		if(nbuffer>=(maxbufferhead-1)){
			cout << this << " warning nbuffer > maxbufferhead "
			<< nbuffer << " " << maxbufferhead << endl;
			nbuffer = (maxbufferhead-1);
		}

		int beginaddr = nbuffer * numbuffersamples;

		for(int i=0; i<numbuffersamples;i++){
			bufferstr[i] = fullbufferstr[beginaddr+i];
		}

		return bufferstr;
}




   void threadedFunction(){

   		while (isThreadRunning()) {

			if(systemcall.size()<=1){
					cout << this << " systemcall empty " << endl;
					stopThread();
			}

			// typedef struct  cst_wave_struct {
			//     const char *type;
			//     int sample_rate;
			//     int num_samples;
			//     int num_channels;
			//     short *samples;
			// } cst_wave;

			// flite_text_to_speech(systemcall.c_str(),flitevoice,"play");
			cst_wave * wav = flite_text_to_wave(systemcall.c_str(),flitevoice);
			runningnumsamples = wav->num_samples;
			flitesamplerate = wav->sample_rate;

			// cout << "samples " << runningnumsamples <<  " sr "<<flitesamplerate << endl;
			if(runningnumsamples!=numbuffersamples){
				fullbufferstr.resize(runningnumsamples,'\0');
				cout << "resized samples to " << runningnumsamples << endl;

			}
			for(int i=0; i<runningnumsamples; i++){ // short to char
				fullbufferstr[i] = (char) ofMap(wav->samples[i],-32767,32767,-127,127);
			}

			stopThread();

		} // while thread running

	} // func


};







class s373AVSpeak {
protected:

	s373AVSpeakT speakthread;
	std::string	systemcall;
	float speakspeed, volume;
	int samplerate,buffersize;
	int voiceom;
	std::vector<float> audiovec;
	Poco::FastMutex mute;

public:
	~s373AVSpeak(){}
	s373AVSpeak(){}


	void setup(int sr, int bs, std::string call="hello world",
	float speed=1, float vol = 1, bool doloop = true, int vom = 0) {

		samplerate = sr;
		buffersize = bs;
		systemcall = call;
		volume = vol;
		audiovec.assign(buffersize,0.f);
		setSpeakSpeed(speed);
		speakthread.setup(sr,bs,call,speed,doloop,vom);

	}


	s373AVSpeak*  setVoice(int n){
		speakthread.setNewVoice(n);
		return this;
	}

	s373AVSpeak*  setText(const std::string & t){
		systemcall=t;
		speakthread.setSystemCall(t);
		// speakthread.setLoop(false);
		speakthread.setBufferLocPercent(0.0f);
		return this;
	}

	s373AVSpeak*  loadFile(const std::string & t){
		systemcall = speakthread.loadFile(t);
		speakthread.setSystemCall(systemcall);
		return this;
	}

	s373AVSpeak* setVolume(float v) { volume = v; return this;}


	inline void calcreaderdata(){

		const std::string & data = speakthread.readStr(buffersize);

		mute.lock();
		for(int i=0; i<buffersize;i++){
			audiovec[i] = ofMap(data[i],-127,127,-volume,volume);
		}
		mute.unlock();

	}

	s373AVSpeak* setSpeakSpeed(float speed){
		speakspeed=speed;
		speakthread.setSpeakSpeed(speed);
		return this;
	}

	float getSpeakSpeed(){
		return speakspeed;
	}
	s373AVSpeak* setLoop(bool b){
		speakthread.setLoop(b);
		return this;
	}

	bool getLoop(){
		return speakthread.getLoop();
	}

	s373AVSpeak* setBufferLocPercent(float per){
		speakthread.setBufferLocPercent(per);
		return this;
	}

	float getBufferLocPercent(){
		return speakthread.getBufferLocPercent();
	}

	s373AVSpeak* setMinMaxBufferLoc(float min, float max){
		speakthread.setMinMaxBufferLoc(min,max);
		return this;
	}

	/*virtual*/// s373AChannel * processBuffer(  float inmastervol=1.0f  ){
	float * processBuffer( void ){

		calcreaderdata();
		return &audiovec[0];

	}

	bool isThreadRunning(){
		return speakthread.isThreadRunning();
	}


	void draw(int x, int y, int nchars=25){

		std::string info ="";

		int strsize = systemcall.size()-1;

		if(nchars >= strsize){
			nchars = strsize;
		}

		float bufferpercent = speakthread.getBufferLocPercent();

		int halfchars = nchars / 2;

		int halfstrpos = (int) (bufferpercent * (float)strsize );
		int strpos = halfstrpos - halfchars;
		if(strpos<0){ strpos = 0; }

		for(int i=0; i<nchars; i++){
			int idx = strpos + i;
			if(idx >= (strsize)){
				idx -= strsize;
			}

			info += systemcall[idx];

		}

		if(mute.tryLock(20)){
			for(int i=0; i<buffersize-4;i+=3){
				ofLine(
					ofMap(i,0,buffersize,0,ofGetWidth()),
					ofMap(audiovec[i],1,-1,0,ofGetHeight()),
					ofMap(i+3,0,buffersize,0,ofGetWidth()),
					ofMap(audiovec[i+3],1,-1,0,ofGetHeight()) );
			}
			mute.unlock();
			ofDrawBitmapString(info, x, y);
		}


	}



};


typedef s373AVSpeak ofxFlite;
