// The Program Original Programmed By B.Xu  MFCC
// Revised by ZhangLiang with pitch extraction
// Finally Revised by B.Xu for the Processing of 863 Speech 
// of Continuous Speech Recognition on July.12 1997

// Redesigned individually for FDT,WZY,WRH,HTY format
// This is designed for FDT,read data directly from optical disk

//Modified By GS. Jan 4, 1998
//
// File Name: feature.cpp
//
#include "float.h"
#include "math.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "io.h"
#include "assert.h"
#include <conio.h>

#include	"include\const.h"
#include	"include\pitchExtractor.h"	
#include	"include\extern.h"
//

#define	CEPSTRAL_NORMALIZE

//#define  TEST_863
//#define	 TEST_intel
#define	 TEST_record_ourself

#define	LOG_ENERGY

extern int totalFrames;
//
float		 FiltCoe1[FFTLen / 2 + 1], FiltCoe2[FFTLen/2 + 1], En[FiltNum + 1];
short		 Num[FFTLen/2 + 1], iZero;

float	hamming[FrameLen+1];
float   e[MAX_SPEECH_LENGTH],e_max;
float	fe[MAX_SPEECH_LENGTH][PEDE];

float 	e_c[MAX_SPEECH_LENGTH];
short	c[MAX_SPEECH_LENGTH];

void	GenHamming();
void	InitFilt(float *,float *,short *);
void	CheckEndPoint(short & ,short & ,short);
void	Cepstrum(void);
short	CFilt(float *,float *,float *,short *,float *);
void	FFT(float *,float *,short);
void	MFCC(float *, short &);
void	DifFeature(void);

short	*hp_speech;


extern int		speechDataLength;
short int		sPos, ePos;


///与实时系统对比测试
void	FeatureExtractT(char *fileName)
{
	// Read Out a Test File for Testing
	short	i, j;
	short	buffer[hFrameLen];
	FILE	*fhwave;
	float	pitchBuf[MAX_SPEECH_LENGTH];

	// Open speech data file 
//	if((fhwave = fopen("cep.dat", "rb")) == NULL)
	if((fhwave = fopen(fileName, "rb")) == NULL)
	{
		printf("Speech Data file open error !\n");
		exit(-1);
	}

	/*//read cepstral feature from file and do not calculate it
	speechDataLength=300;
	for(i=0;i<speechDataLength;i++)
		fread(pVector[i], sizeof(float), No_Dim, fhwave);
	fclose(fhwave);
	fhwave = fopen("cepTrue.dat", "rb");
	float vec[No_Dim];
	for(i=0;i<speechDataLength;i++)
	{
		fread(vec,sizeof(float),No_Dim,fhwave);
		if(i>200)
			for(j=0;j<No_Dim;j++)
			if(j!=0||j!=14||j!=28||j!=13||j!=27||j!=41)
			pVector[i][j] = vec[j];
		
	}
	*/
	fseek(fhwave, 0, SEEK_END);	

#ifdef TEST_863
	speechDataLength = ftell(fhwave) - 44; //for 863 wave format
	speechDataLength = speechDataLength / 2 / hFrameLen;
	speechDataLength--;
	fseek(fhwave, 44, SEEK_SET);	
#endif

#ifdef TEST_intel
	speechDataLength = ftell(fhwave) - 300; //for intel beijing format
	speechDataLength = speechDataLength / 2 / hFrameLen;
	speechDataLength--;
	fseek(fhwave, 300, SEEK_SET);	
#endif

#ifdef TEST_record_ourself
	speechDataLength = ftell(fhwave);
	speechDataLength = speechDataLength / 2 / hFrameLen;
	speechDataLength--;
	fseek(fhwave, 0, SEEK_SET);
#endif

	if(speechDataLength > MAX_SPEECH_LENGTH)
	{
		printf("Too long speech!");
		getch();
		exit(-1);
	}

	///
	memset(hp_speech,0,sizeof(short)*hFrameLen);
	///
	for(i = 0; i <= speechDataLength; i++)
	{
		fread(buffer, sizeof(short), hFrameLen, fhwave);
		for(j = 0; j < hFrameLen; j++)
//			hp_speech[(long)i * hFrameLen + (long)j] = buffer[j];
		hp_speech[(long)(i+1) * hFrameLen + (long)j] = buffer[j];
	}
	memset(hp_speech+(speechDataLength+2)*hFrameLen,0,sizeof(short)*hFrameLen);
	// Close speech data file
	fclose(fhwave);
	//extract pitch
	speechDataLength++;
	SetPitchExchangeBuf(hp_speech, (speechDataLength+1)*hFrameLen, pitchBuf);
//	SetPitchExchangeBuf(hp_speech, speechDataLength*hFrameLen, pitchBuf);
	ExtractPitch();
	// Make EndPoint Detection Into Several Pieces
	
	CheckEndPoint(sPos, ePos, speechDataLength);
	// extract Cepstrum feature parametres
	
#ifdef TEST_record_ourself
	ePos = speechDataLength - 1;
	//ePos = speechDataLength - 2;
	sPos = 0;
#endif
	speechDataLength = ePos - sPos + 1;

/*/
	char name[200];
	int len=strlen(fileName);
	strcpy(name,fileName);
	name[len-1]='t';
	name[len-2]='a';
	name[len-3]='d';
	fhwave = fopen(name, "wb");	
	if((speechDataLength+1)%20)
		speechDataLength=speechDataLength + 20 - (speechDataLength % 20);
	fwrite(hp_speech+sPos*hFrameLen,sizeof(short),speechDataLength*hFrameLen, fhwave);
	fclose(fhwave);
	return;
*///

	Cepstrum();	
//
	totalFrames += speechDataLength;

#ifdef CEPSTRAL_NORMALIZE
	float averageCepstral[FEATURE_DIM];
	for(i = 0; i < FEATURE_DIM; i++)
		averageCepstral[i] = 0;
	for(i = 0; i < speechDataLength; i++)
		for(j = 0; j < FEATURE_DIM; j++)
			averageCepstral[j] += pVector[i][j];
	for(i = 0; i < FEATURE_DIM; i++)
		averageCepstral[i] /= speechDataLength;
	for(i = 0; i < speechDataLength; i++)
		for(j = 1; j < FEATURE_DIM-1; j++)
			pVector[i][j] -= averageCepstral[j];
#endif

	for(i = 0; i < speechDataLength; i++)
	{
#ifdef PITCH_LOG
		pVector[i][FEATURE_DIM-1] = log10(pitchBuf[sPos+i]);
#else
		pVector[i][FEATURE_DIM-1] = pitchBuf[sPos+i];
#endif
	}
	DifFeature();

//fhwave = fopen("cepAvg.dat", "wb");
//for(i=0;i<speechDataLength;i++)
//{
	//fwrite(pVector[i], sizeof(float), No_Dim, fhwave);
//	fwrite(averageCepstral, sizeof(float), FEATURE_DIM, fhwave);
//	for(j=0;j<FEATURE_DIM;j++)
//		fprintf(fhwave, "%f ", pVector[i][j]);
//	fprintf(fhwave, "\n");
//}
//fclose(fhwave);

	//
	FILE*fp=fopen("cep.dat","wb");
	for(i=0;i<speechDataLength;i++)
	{
			fwrite(pVector[i],sizeof(float),No_Dim,fp);
	}
	fclose(fp);

	fclose(fhwave);


}

//
extern void	FeatureExtract(char *fileName)
{
	// Read Out a Test File for Testing
	short	i, j;
	short	buffer[hFrameLen];
	FILE	*fhwave;
	float	pitchBuf[MAX_SPEECH_LENGTH];

	// Open speech data file 
	if((fhwave = fopen(fileName, "rb")) == NULL)
	{
		printf("Speech Data file open error !\n");
		exit(-1);
	}

	fseek(fhwave, 0, SEEK_END);	

#ifdef TEST_863
	speechDataLength = ftell(fhwave) - 44; //for 863 wave format
	speechDataLength = speechDataLength / 2 / hFrameLen;
	speechDataLength--;
	fseek(fhwave, 44, SEEK_SET);	
#endif

#ifdef TEST_intel
	speechDataLength = ftell(fhwave) - 300; //for intel beijing format
	speechDataLength = speechDataLength / 2 / hFrameLen;
	speechDataLength--;
	fseek(fhwave, 300, SEEK_SET);	
#endif

#ifdef TEST_record_ourself
	speechDataLength = ftell(fhwave);
	speechDataLength = speechDataLength / 2 / hFrameLen;
	speechDataLength--;
	fseek(fhwave, 0, SEEK_SET);
#endif

	if(speechDataLength > MAX_SPEECH_LENGTH)
	{
		printf("Too long speech!");
		getch();
		exit(-1);
	}

	for(i = 0; i <= speechDataLength; i++)
	{
		fread(buffer, sizeof(short), hFrameLen, fhwave);
		for(j = 0; j < hFrameLen; j++)
			hp_speech[(long)i * hFrameLen + (long)j] = buffer[j];
	}
	// Close speech data file
	fclose(fhwave);
	//extract pitch
	SetPitchExchangeBuf(hp_speech, speechDataLength*hFrameLen, pitchBuf);
	ExtractPitch();
	// Make EndPoint Detection Into Several Pieces
	
	CheckEndPoint(sPos, ePos, speechDataLength);
	// extract Cepstrum feature parametres
	
#ifdef TEST_record_ourself
	ePos = speechDataLength - 2;
	sPos = 0;
#endif
	speechDataLength = ePos - sPos + 1;

	Cepstrum();	
	totalFrames += speechDataLength;

#ifdef CEPSTRAL_NORMALIZE
	float averageCepstral[FEATURE_DIM];
	for(i = 0; i < FEATURE_DIM; i++)
		averageCepstral[i] = 0;
	for(i = 0; i < speechDataLength; i++)
		for(j = 0; j < FEATURE_DIM; j++)
			averageCepstral[j] += pVector[i][j];
	for(i = 0; i < FEATURE_DIM; i++)
		averageCepstral[i] /= speechDataLength;
	for(i = 0; i < speechDataLength; i++)
		for(j = 1; j < FEATURE_DIM-1; j++)
			pVector[i][j] -= averageCepstral[j];
#endif

	for(i = 0; i < speechDataLength; i++)
	{
#ifdef PITCH_LOG
		pVector[i][FEATURE_DIM-1] = log10(pitchBuf[sPos+i]);
#else
		pVector[i][FEATURE_DIM-1] = pitchBuf[sPos+i];
#endif
	}
	DifFeature();
}


/*
This function is to implement end-point-checking
*/
void CheckEndPoint(short &sPos, short &ePos, short tsl)
{
	short		i, j, flag, No_speech;
	long   		ptr;
	float 		e_lim, e_c_lim;

	// Calculate the Eneryg for Every Frame
	ptr = 0L;
	e_max = 0;

	int energyBias = 0;
	//
	for(i = 0; i < tsl; i++) //Jan 13
	{
		for(j = 0; j < hFrameLen; j++)
		{
			energyBias += hp_speech[ptr];
			ptr++;
		}
	}

	energyBias /= ptr;

	ptr = 0;
	for(i = 0; i < tsl; i++) //Jan 13
	{
		for(j = 0; j < hFrameLen; j++)
		{
			hp_speech[ptr] -= energyBias;
			ptr++;
		}
	}
	ptr = 0;


	for(i = 0; i < tsl; i++) //Jan 13
	{
		e[i] = 0;
		c[i] = 0;
	
		for(j = 0; j < hFrameLen; j++)
		{
			e[i] += ((float)hp_speech[ptr] * (float)hp_speech[ptr]); //energy
			if(hp_speech[ptr] * hp_speech[ptr+1] < 0)
				c[i]++;  //cross-zero

			ptr++;
		}
		e_c[i] = e[i] * (float)c[i]; //energy &cross-zero
	}

	// Estimate the Background noise
	e_lim = 0;
	e_c_lim = 0;
	for(i = 0; i < 4; i++)
	{
		e_lim += e[i];
		e_c_lim += e_c[i];
	}
	
	e_lim = e_lim / 4;
	e_c_lim = e_c_lim / 4;

	// Check the Every segment of words one by one
	flag = 0;
	No_speech = 0;
	sPos = 0;
	ePos = tsl / 2 + 2;

	
	for(i = 5; i < tsl / 2; i++)
	{
		if(e[i] >= 4 * e_lim)
			if(e[i+1] >= 4 * e_lim)
				if(e[i+2] >= 4 * e_lim)
					if(e[i+3]>=4 * e_lim)
						{
							sPos = i - 2;
							break;
						}
	}

	for(i = tsl - 5; i >= tsl / 2; i--)
	{
		if(e[i] >= 4 * e_lim)
			if(e[i-1] >= 4 * e_lim)
				if(e[i-2] >= 4 * e_lim)
					if(e[i-3] >= 4 * e_lim)
					{
						ePos = i+2;
						break;
					}
	}
}	

/*
This function is to extract Cesptrum Parameters.
*/
void Cepstrum()
{   

	long	ptr;
	short	i, t, tmp;
	float	fsmp[FrameLen], dat10, dat11, date;

	e_max = 0.0;

	for(t = sPos; t <= ePos; t++)//Jan 13
	{
		date = e[t] + e[t+1];
		if(date > e_max)
			e_max = date;
	}
	
	for(t = sPos; t <= ePos; t++)//Jan 14
	{
		for(i = 0; i < PCEP + 1; i++)
			pVector[t-sPos][i] = 0.0;
		
		fe[t-sPos][0] = (e[t] + e[t+1]) / e_max;
		//add energy to pVector
		pVector[t-sPos][0] = fe[t-sPos][0];

		ptr = (long)(t) * hFrameLen;

		for(i = 0; i < FrameLen; i++)     // hamming windowed  &  pre-emphasize
		{
			dat10 = (float)hp_speech[ptr] * hamming[i];
			dat11 = (float)hp_speech[ptr+1] * hamming[i+1];
			fsmp[i] = dat11 - pre * dat10;
			ptr++;
		}

	    iZero = CFilt(fsmp, FiltCoe1, FiltCoe2, Num, En);
	    if(iZero == -1)// && j > 5)
		{
			printf("Energy is Zero in Processing");
			exit(-1);
		}
		
		tmp = t - sPos;
	    MFCC(En, tmp);
	}
}


/*
This function is to calculate Hamming Window Funtion
*/
extern void GenHamming()
{
	short i;
	double twopi;

	twopi = 8.* atan(1.0);
	for(i = 0; i < FrameLen + 1; i++)
	hamming[i] = (float)(0.54 - 0.46 * cos((float)i * twopi / (float)(FrameLen - 1)));
}

/*
This function is to initialize the fileter of MEL parameters
*/
extern void InitFilt(float *FiltCoe1, float *FiltCoe2, short *Num)
{
	short	i,j;
	float	Freq;
	short	FiltFreq[FiltNum+1] = {0,100,200,300,400,500,600,700,800,900,1000,
								   1149,1320,1516,1741,2000,2297,2639,3031,3482,
								   4000,4595,5278,6063,6964,8001};
	short	BW[FiltNum+1] = {100,100,100,100,100,100,100,100,100,100,124,
							 160,184,211,242,278,320,367,422,484,556,
							 639,734,843,969,1112};

	for(i = 0; i <= FFTLen / 2; i++)
	{
		Freq = FS * 1000.0 * (float)(i)/(float)(FFTLen);
		for(j = 0; j < FiltNum; j++)
		{
			if(Freq >= (float)FiltFreq[j] && Freq <= (float)FiltFreq[j+1])
			{
				Num[i] = j;
				if(j == 0)
					FiltCoe1[i] = 0.0;
				else
					FiltCoe1[i] = ((float)(FiltFreq[j] + BW[j]) - Freq) / (float)(BW[j]);

				FiltCoe2[i] = (Freq - (float)(FiltFreq[j+1] - BW[j+1])) / (float)(BW[j+1]);
				FiltCoe1[i] = FiltCoe1[i] * FiltCoe1[i];
				FiltCoe2[i]=FiltCoe2[i] * FiltCoe2[i];
				break;
			}
		}
	}
}

/*              Get energy from triangle
filter                                             */
/*    spdata[]  :speech
signal                                                    */
/*    FiltCoe1[]:plus K of filter
coefficients                                    */
/*    FiltCoe2[]:minus K of filter
coefficients                                   */
/*    Num[]     :decide the filter that one point
belongs                         */
/*    En[]      :output energy after speech signal is filted by triangle
filters  */

short CFilt(float *spdata,float *FiltCoe1,float *FiltCoe2,short *Num, float *En)
{
	float	real[FFTLen+1], imgi[FFTLen+1], temp;
	short	id, id1, id2;

	for(id = 0; id < FFTLen; id++)
	{
		if(id < FrameLen)
			real[id+1] = spdata[id];
		else
			real[id+1] = 0.0;
		imgi[id+1] = 0.0;
	}
	FFT(real, imgi, 0);

	for(id = 0; id <= FiltNum; id++)
		En[id] = 0.0;
	for(id=0; id <= FFTLen / 2; id++)
	{
		temp = real[id+1] * real[id+1] + imgi[id+1] * imgi[id+1];
		id1 = Num[id];
		id2 = id1 + 1;
		En[id1] = En[id1] + FiltCoe1[id] * temp;
		En[id2] = En[id2] + FiltCoe2[id] * temp;
	}

	for(id = 1; id <= FiltNum; id++)
	{
		if(En[id] == 0)
			return(-1);
		else
			En[id] = log(En[id]);
	}
	return(0);
}

/*              get mel_frequency
cepstrum                                 */
/*                                                                        
*/
/*    En[]  : energy after speech signal is filted by the triangle
filters */
/*    Cep[] : cepstrum
coefficent                                          */
/*    Ceplen: lengthen of cepstrum
coefficent                              */

void MFCC(float *En,short & tidx)
{
	short idcep,iden;

	for(idcep = 0; idcep < PCEP; idcep++)
	{
		pVector[tidx][idcep+1] = 0.0;

		for(iden = 1; iden <= FiltNum; iden++)
			pVector[tidx][idcep+1] = pVector[tidx][idcep+1] + En[iden] * cos((idcep + 1) * (iden - 0.5) * PI / (FiltNum));
	
		pVector[tidx][idcep+1] = pVector[tidx][idcep+1] / 10.0;
	}
}

/*This is function is to carry out FFT */
void FFT(float *xr, float *xi, short inv)
{
	short	m, nv2, i, j, nm1, k, l, le1, ip, n = FFTLen;
	float	tr, ti, ur, ui, wr, wi, ur1, ui1;
	
	m = (short)(log((double)n) / log(2.) +.1);
	nv2 = n / 2;
	nm1 = n-1;
	j = 1;
	for(i = 1; i <= nm1; ++i)
	{
		if(i < j)
		{
			tr=xr[j];
			ti=xi[j];
			xr[j]=xr[i];
			xi[j]=xi[i];
			xr[i]=tr;
			xi[i]=ti;
		}
		k=nv2;
		R20:
			if(k >= j) goto R30;
			j = j - k;
		k = (short)(k / 2);
		goto R20;
		R30: 
			j = j + k;
	}
	for(l = 1; l <= m; ++l)
	{
		le1 = (short)(pow(2, (l-1)));
		ur = 1.0f;
		ui = 0.f;
		wr = (float)cos(PI / (float)(le1));
		wi = -(float)sin(PI / (float)(le1));
		if(inv != 0)
			wi = -wi;
		for(j = 1; j <= le1; ++j)
		{
			for(i = j; i <= n; i = i + 2 * le1)
			{
				ip = i + le1;
				tr = xr[ip] * ur - xi[ip] * ui;
				ti = xr[ip] * ui + xi[ip] * ur;
				xr[ip] = xr[i] - tr;
				xi[ip] = xi[i] - ti;
				xr[i] = xr[i] + tr;
				xi[i] = xi[i] + ti;
			}
			ur1 = ur * wr - ui * wi;
			ui1 = ur * wi + ui * wr;
			ur = ur1;
			ui = ui1;
		}
	}
	if(inv == 0)
		return;
	for(i = 1; i <= n; ++i)
	{
		xr[i] = xr[i] / (float)(n);
		xi[i] = xi[i] / (float)(n);
	}
}

void DifFeature()
{
	int	j, ii;

#ifdef	LOG_ENERGY
	for(j = 0; j < speechDataLength; j++)
	{
		if(pVector[j][0]<1.0e-5)
			pVector[j][0]=1.0e-5;
		pVector[j][0]=log10(pVector[j][0])/5.0;
	}
#endif

	// Calculate Differential Feature
	for(j = 2; j < speechDataLength - 2; j++)
		for(ii = 0; ii < FEATURE_DIM; ii++)
			pVector[j][ii+FEATURE_DIM] = pVector[j+2][ii] - pVector[j-2][ii];
			
	for(ii = 0; ii < FEATURE_DIM; ii++)
    {
		pVector[speechDataLength-2][ii+FEATURE_DIM] = pVector[speechDataLength-3][ii+FEATURE_DIM];
		pVector[speechDataLength-1][ii+FEATURE_DIM] = pVector[speechDataLength-3][ii+FEATURE_DIM];
    }
			
	for(ii = 0; ii < FEATURE_DIM; ii++)
	{
		pVector[0][ii+FEATURE_DIM] = pVector[2][ii+FEATURE_DIM];				
		pVector[1][ii+FEATURE_DIM] = pVector[2][ii+FEATURE_DIM];				
	}

	// calculate 2 differential energy
	for(j = 2; j < speechDataLength - 2; j++)
		for(ii = 0; ii < FEATURE_DIM; ii++)
			pVector[j][ii+2*FEATURE_DIM] = pVector[j+2][ii+FEATURE_DIM] - pVector[j-2][ii+FEATURE_DIM];
			
	for(ii = 0; ii < FEATURE_DIM; ii++)
    {
		pVector[speechDataLength-2][ii+2*FEATURE_DIM] = pVector[speechDataLength-3][ii+2*FEATURE_DIM];
		pVector[speechDataLength-1][ii+2*FEATURE_DIM] = pVector[speechDataLength-3][ii+2*FEATURE_DIM];
    }
			
	for(ii=0;ii<FEATURE_DIM;ii++)
	{
		pVector[0][ii+2*FEATURE_DIM] = pVector[2][ii+2*FEATURE_DIM];				
		pVector[1][ii+2*FEATURE_DIM] = pVector[2][ii+2*FEATURE_DIM];				
	}
}