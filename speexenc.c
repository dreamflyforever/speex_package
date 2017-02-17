#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <speex/speex.h>

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

static int _get_framesize(int samplerate)
{
	return (samplerate * 2)/ (1000/20); /* 20ms */
}

static int _write_header(uint8_t *frame, int pdu_len, int headersz)
{
	switch (headersz) {
	case 1:
		*frame = pdu_len;
		break;

	case 2:
		*(frame + 0) = pdu_len & 0xff;
		*(frame + 1) = (pdu_len >> 8) & 0xff;
		break;

	case 4:
		*(frame + 0) = pdu_len & 0xff;
		*(frame + 1) = (pdu_len >> 8) & 0xff;
		*(frame + 2) = (pdu_len >> 16) & 0xff;
		*(frame + 3) = (pdu_len >> 24) & 0xff;
		break;
	}

	return 0;
}

spx_int16_t pcm_frame[320];
uint8_t spx_frame[128];

int main(int argc, char** argv)
{
	int fd, out_fd;
	int frame_size;

	int speex_headersz;

	struct wav_header header;
	void *speex = NULL;
	void *pre = NULL;

	void *st = 0;
	SpeexBits bits;

	if ((argc != 2) && (argc != 3)) {
		printf("usage: speexenc in_wav_file [out_speex_file]\n");
		return -1;
	}

	speex_headersz = 2;

	fd = open(argv[1], O_RDONLY, 0);
	if (fd < 0) {
		printf("open wav failed!\n");
		return -1;
	}

	if (argc == 3)
		out_fd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
	else
		out_fd = open("dummy.spx", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (out_fd < 0) {
		printf("open speex file failed!\n");
		close(fd);
		return -1;
	}

	/* read riff header */
	read(fd, &header, sizeof(struct wav_header));
	frame_size = _get_framesize(header.samplespersec);

	{
		int mode;

		if (header.samplespersec > 12500) mode = SPEEX_MODEID_WB;
		else mode = SPEEX_MODEID_NB;

		st = speex_encoder_init(speex_lib_get_mode(mode));
	}

	{
		spx_int32_t tmp;

		tmp = 1;
		speex_encoder_ctl(st, SPEEX_SET_COMPLEXITY, &tmp);
		speex_encoder_ctl(st, SPEEX_SET_DTX, &tmp);
		speex_encoder_ctl(st, SPEEX_SET_VBR, &tmp);

		tmp = 8;
		speex_encoder_ctl(st, SPEEX_SET_QUALITY, &tmp);
		tmp = header.samplespersec;
		speex_encoder_ctl(st, SPEEX_SET_SAMPLING_RATE, &tmp);
	}

	/* Speex encoding initializations */
	speex_bits_init(&bits);

	while (1) {
		/* hand one frame */
		int length = read(fd, pcm_frame, frame_size);
		if (length != frame_size) break;

		speex_bits_reset(&bits);
		speex_encode_int(st, pcm_frame, &bits);
		length = speex_bits_write(&bits,
				&spx_frame[speex_headersz],
				sizeof(spx_frame) - speex_headersz);

		_write_header(spx_frame, length, speex_headersz);
		write(out_fd, spx_frame, length + speex_headersz);
	}
	speex_encoder_destroy(st);
	speex_bits_destroy(&bits);
	close(fd);
	close(out_fd);

	return 0;
}
