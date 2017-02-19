#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <speex/speex.h>

spx_int16_t pcm_frame[320];

struct wav_header
{
	char riff_id[4];			/*("RIFF"*/
	uint32_t size0;				/*file len - 8*/
	char wave_fmt[8];			/*"WAVEfmt "*/
	uint32_t size1;				/*0x10*/
	uint16_t fmttag;			/*0x01*/
	uint16_t channel;			/*1*/
	uint32_t samplespersec;			/*8000*/
	uint32_t bytepersec;			/*8000 * 2*/
	uint16_t blockalign;			/*1 * 16 / 8*/
	uint16_t bitpersamples;			/*16*/
	char data_id[4];			/*"data"*/
	uint32_t size2;				/*file len - 44*/
};

static void init_wav_header(char *wav_buf, uint32_t wav_len, int samplerate)
{
	struct wav_header *header = (struct wav_header *)wav_buf;

	strcpy(header->riff_id, "RIFF");
	header->size0 = wav_len - 8;
	strcpy(header->wave_fmt, "WAVEfmt ");
	header->size1 = 16;
	header->fmttag = 1;
	header->channel = 1;
	header->samplespersec = samplerate;
	header->bytepersec = samplerate * 2;
	header->blockalign = 2;
	header->bitpersamples = 16;
	strcpy(header->data_id, "data");
	header->size2 = wav_len - 44;
}

static int get_header_length(uint8_t *data, int header_len)
{
	int length = 0;

	if (header_len == 1) length = *data;
	if (header_len == 2) {
		length = *data | *(data + 1) << 8;
	}
	if (header_len == 4) {
		length = *data | *(data + 1) << 8 | *(data + 2)
		<< 16 | *(data + 3) << 24;
	}

	return length;
}

int main(int argc, char *argv[])
{
	int header_len = 2;
	int samplerate = 16000;
	int length;
	int out_fd;
	int pcm_length = 0;
	int sample_num = samplerate / (1000 / 20);
	uint8_t spx_data[128] = {0};
	int frame_len;
	void *stateDecode;
	SpeexBits bitsDecode;
	struct wav_header header;
	int mode;
	int in_fd;

	if ((argc > 4) || (argc < 2)) {
		printf("usage: speexdec in_speex_file [out_wav_file]");
		return -1;
	}

	in_fd = open(argv[1], O_RDONLY, 0666);
	if (in_fd < 0) {
		printf("open %s error\n", argv[1]);
		return -1;
	}

	if (argc == 3) out_fd = open(argv[2],
					O_WRONLY | O_CREAT | O_TRUNC, 0666);
	else out_fd = open("dummy.wav", O_WRONLY | O_CREAT | O_TRUNC, 0666);

	if (out_fd < 0) {
		printf("open wav file error\n");
		return -1;
	}

	if (samplerate > 12500) mode = SPEEX_MODEID_WB;
	else mode = SPEEX_MODEID_NB;

	/*printf("speex decode mode: %s\n",
		mode == SPEEX_MODEID_WB ? "WB" : "NB");*/
	/*printf("speex sample rate: %d\n", samplerate);*/

	stateDecode = speex_decoder_init(speex_lib_get_mode(mode));
	speex_bits_init(&bitsDecode);

	/* write header firstly */
	init_wav_header((char *)&header, 0, 0);
	write(out_fd, &header, sizeof(struct wav_header));

	while (1) {
		/* read header */
		length = read(in_fd, spx_data, header_len);
		if (length != header_len) break;
		frame_len = get_header_length(spx_data, header_len);
		length = read(in_fd, spx_data, frame_len);
		if (length != frame_len) break;

		speex_bits_reset(&bitsDecode);
		speex_bits_read_from(&bitsDecode, spx_data, frame_len);

		int ret = speex_decode_int(stateDecode,
				&bitsDecode,
				(spx_int16_t*)pcm_frame);

		write(out_fd, pcm_frame, sample_num * sizeof(uint16_t));
		pcm_length += sample_num * sizeof(uint16_t);
	}
	close(in_fd);

	speex_bits_destroy(&bitsDecode);
	speex_decoder_destroy(stateDecode);

	/* rewrite wav header */
	lseek(out_fd, 0, SEEK_SET);
	init_wav_header((char *)&header, pcm_length, samplerate);
	write(out_fd, &header, sizeof(struct wav_header));
	close(out_fd);
}
