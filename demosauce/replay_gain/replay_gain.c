#include <stdlib.h>
#include <string.h>
#include "replay_gain.h"
#include "gain_analysis.h"

struct _RG_CONTEXT 
{
	RG_SampleFormat format;
	Context_t * cxt;
	void * buffer;
	size_t bufferSize;
};

RG_Context * RG_NewContext(RG_SampleFormat * format)
{
	Context_t * cxt = NewAnalyzeContext();
	int val = InitGainAnalysis(cxt, format->sampleRate);
	if (val == INIT_GAIN_ANALYSIS_ERROR)
	{
		FreeAnalyzeContext(cxt);
		return NULL;
	}
	RG_Context * context = malloc(sizeof(RG_Context));
	memset(context, 0, sizeof(RG_Context));
	context->format = *format;
	context->cxt = cxt;
	context->buffer = NULL;
	context->bufferSize = 0;
	return context;
}

void RG_FreeContext(RG_Context * context)
{
	FreeAnalyzeContext(context->cxt);
	free(context->buffer);
}

size_t RG_FormatSize(uint32_t sampleFormat)
{
	switch (sampleFormat) 
	{
		case RG_UNSIGNED_8_BIT: return sizeof(uint8_t);
		case RG_SIGNED_16_BIT: return sizeof(int16_t);
		case RG_FLOAT_32_BIT: return sizeof(float);
		case RG_FLOAT_64_BIT: return sizeof(double);
		default: return 0; // hmm what is good value here?
	}
}

void UpdateBuffer(RG_Context * context, uint32_t length)
{
	const size_t requiredSize = length * sizeof(Float_t) * context->format.numberChannels;
	if (context->bufferSize < requiredSize)
		context->buffer = realloc(context->buffer, requiredSize);
}

void ConvertF64(RG_Context * context, void * data, uint32_t length)
{
	// the lib expexts the data in int_16 range
	static Float_t const buttScratcher = 0x7fff; 
	if (context->format.numberChannels == 2)
	{
		double const *  in = (double *) data;
		Float_t * outl = (Float_t *) context->buffer;
		Float_t * outr = (Float_t *) context->buffer + context->bufferSize / 2;
		for (size_t i = 0; i < length; i++)
		{
			*outl++ = (Float_t) *in++ * buttScratcher;
			*outr++ = (Float_t) *in++ * buttScratcher;
		}
	}
		else if (context->format.numberChannels == 1)
	{
		double const *  in = (double *) data;
		Float_t * out = (Float_t *) context->buffer;
		for (size_t i = 0; i < length; i++)
			*out++ = (Float_t) *in++ * buttScratcher;
	}
}

void ConvertF32(RG_Context * context, void * data, uint32_t length)
{
	// the lib expexts the data in int_16 range
	static Float_t const buttScratcher = 0x7fff; 
	if (context->format.numberChannels == 2)
	{
		float const *  in = (float *) data;
		Float_t * outl = (Float_t *) context->buffer;
		Float_t * outr = (Float_t *) context->buffer + context->bufferSize / 2;
		for (size_t i = 0; i < length; i++)
		{
			*outl++ = (Float_t) *in++ * buttScratcher;
			*outr++ = (Float_t) *in++ * buttScratcher;
		}
	}
	else if (context->format.numberChannels == 1)
	{
		float const *  in = (float *) data;
		Float_t * out = (Float_t *) context->buffer;
		for (size_t i = 0; i < length; i++)
			*out++ = (Float_t) * in++ * buttScratcher;
	}
}

void ConvertS16(RG_Context * context, void * data, uint32_t length)
{
	//static Float_t const buttScratcher = 1.0 / 0x8000;
	if (context->format.numberChannels == 2)
	{
		int16_t const *  in = (int16_t *) data;
		Float_t * outl = (Float_t *) context->buffer;
		Float_t * outr = (Float_t *) context->buffer + context->bufferSize / 2;
		for (size_t i = 0; i < length; i++)
		{
			*outl++ = (Float_t) *in++; // * buttScratcher;
			*outr++ = (Float_t) *in++; // * buttScratcher;
		}
	}
	else if (context->format.numberChannels == 1)
	{
		int16_t const *  in = (int16_t *) data;
		Float_t * out = (Float_t *) context->buffer;
		for (size_t i = 0; i < length; i++)
			*out++ = (Float_t) *in++; // * buttScratcher;
	}
}

void ConvertU8(RG_Context * context, void * data, uint32_t length)
{
	static Float_t const buttScratcher = 0x101;
	if (context->format.numberChannels == 2)
	{
		int16_t const *  in = (int16_t *) data;
		Float_t * outl = (Float_t *) context->buffer;
		Float_t * outr = (Float_t *) context->buffer + context->bufferSize / 2;
		for (size_t i = 0; i < length; i++)
		{
			*outl++ = (Float_t) *in++ * buttScratcher - 0x8000;
			*outr++ = (Float_t) *in++ * buttScratcher - 0x8000;
		}
	}
	else if (context->format.numberChannels == 1)
	{
		int16_t const *  in = (int16_t *) data;
		Float_t * out = (Float_t *) context->buffer;
		for (size_t i = 0; i < length; i++)
			*out++ = (Float_t) *in++ * buttScratcher - 0x8000;
	}
}

void RG_Analyze(RG_Context * context, void * data, uint32_t length)
{
	UpdateBuffer(context, length);
	switch (context->format.sampleFormat) 
	{
		case RG_UNSIGNED_8_BIT: ConvertU8(context, data, length); break;
		case RG_SIGNED_16_BIT: ConvertS16(context, data, length); break;
		case RG_FLOAT_32_BIT: ConvertF32(context, data, length); break;
		case RG_FLOAT_64_BIT: ConvertF64(context, data, length); break;
		default: return;
	}
	if (context->format.numberChannels == 2)
		AnalyzeSamples(context->cxt, context->buffer, context->buffer + context->bufferSize / 2, length, 2);
	else if (context->format.numberChannels == 1)
		AnalyzeSamples(context->cxt, context->buffer, NULL, length, 1);
}

double RG_GetTitleGain(RG_Context * context)
{
	double gain = GetTitleGain(context->cxt);
	if (gain == GAIN_NOT_ENOUGH_SAMPLES)
		return 0; // no change
	return gain;
}

double RG_GetAlbumGain(RG_Context * context)
{
	double gain = GetAlbumGain(context->cxt);
	if (gain == GAIN_NOT_ENOUGH_SAMPLES)
		return 0; // no change
	return gain;	
}
