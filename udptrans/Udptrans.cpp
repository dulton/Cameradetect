#include "Udptrans.h"

CUdptrans::CUdptrans(int width ,int height,char*ip,char* port)
	:m_width(width),m_height(height),m_RDIP(ip),m_RDPort(port)
{
	ptParam 		  =	NULL;
	ptCodecCtx	  =    NULL;
	m_pYUVFrame =    new AVFrame[1];
}

CUdptrans::~CUdptrans()
{
}

int CUdptrans::UdptransPre()
{
	av_register_all();
	avcodec_register_all();   
   	avformat_network_init();
   	
	ptFormatCtx = avformat_alloc_context();
	ptFormatCtx->oformat = av_guess_format("h264", NULL, NULL);
	
	snprintf(ptFormatCtx->filename, sizeof(ptFormatCtx->filename),"udp://%s:%d",m_RDIP,atoi(m_RDPort));
	if (avio_open(&ptFormatCtx->pb,ptFormatCtx->filename, AVIO_FLAG_WRITE) < 0){
			printf("Failed to open output file! \n");
			return -1;
	}
	
	ptCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!ptCodec) {
		printf("Codec not found\n");
		return -1;
	}
	
	ptVideoSt = avformat_new_stream(ptFormatCtx, ptCodec);
	ptVideoSt->time_base.num = 1; 
	ptVideoSt->time_base.den = 25;  
	if (ptVideoSt==NULL){
		return -1;
	}
	ptCodecCtx= ptVideoSt->codec;
	avcodec_get_context_defaults3(ptCodecCtx, ptCodec);
	if (!ptCodecCtx) {
		printf( "Could not allocate video codec context\n");
		return -1;
	}
	ptCodecCtx->codec_id 		 = AV_CODEC_ID_H264;

	/* put sample parameters */
	ptCodecCtx->bit_rate 		 = 400000;
	/* resolution must be a multiple of two */
	ptCodecCtx->width 		 = m_width;
	ptCodecCtx->height 		 = m_height;
	/* frames per second */
	ptCodecCtx->time_base 	 = (AVRational){1,25};
	ptCodecCtx->gop_size 		 = 500;
	ptCodecCtx->max_b_frames = 3;
	ptCodecCtx->pix_fmt 		 = AV_PIX_FMT_YUV420P;
	//H264
	//pCodecCtx->me_range = 16;
	//pCodecCtx->max_qdiff = 4;
	//pCodecCtx->qcompress = 0.6;
	ptCodecCtx->qmin = 10;
	ptCodecCtx->qmax = 51;

	if(ptFormatCtx->oformat->flags & AVFMT_GLOBALHEADER)
								ptCodecCtx->flags|= CODEC_FLAG_GLOBAL_HEADER;

	av_opt_set(ptCodecCtx->priv_data, "preset", "ultrafast", 0);
	av_opt_set(ptCodecCtx->priv_data, "tune","stillimage,fastdecode,zerolatency",0);

	if (avcodec_open2(ptCodecCtx, ptCodec,NULL) < 0){
		printf("Failed to open encoder! \n");
		return -1;
	}
	
	avformat_write_header(ptFormatCtx, NULL);

	m_pYUVFrame = av_frame_alloc();
	m_PicSize 	  = avpicture_get_size( AV_PIX_FMT_YUV420P, m_width, m_height);
	m_PicBuf 	  = (uint8_t *)av_malloc(m_PicSize*sizeof(uint8_t));
	m_Ysize 		  = ptCodecCtx->width * ptCodecCtx->height;
	m_pYUVFrame->format = ptCodecCtx->pix_fmt;
	m_pYUVFrame->width   = ptCodecCtx->width;
	m_pYUVFrame->height  = ptCodecCtx->height;
	
	avpicture_fill((AVPicture*)m_pYUVFrame, m_PicBuf, AV_PIX_FMT_YUV420P,  m_width, m_height);
	
	return 0;
}

int CUdptrans::UdptransRun(Mat &img)
{
	int ret;
	int got_output=0;

	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;
	pkt.pts = AV_NOPTS_VALUE;
	pkt.dts = AV_NOPTS_VALUE;
	m_pYUVFrame->pts = ptVideoSt-> codec->frame_number;

	cvtColor(img, YUVFrame, CV_BGR2YUV_I420);

	memcpy(m_PicBuf,YUVFrame.data, m_Ysize*3/2);
	m_pYUVFrame->data[0] = m_PicBuf;              // Y
	m_pYUVFrame->data[1] = m_PicBuf+ m_Ysize;      // U 
	m_pYUVFrame->data[2] = m_PicBuf+ m_Ysize*5/4;  // V

	/* encode the image */
	ret = avcodec_encode_video2(ptCodecCtx ,&pkt, m_pYUVFrame, &got_output);
	if (ret < 0) {
		printf( "Error encoding frame\n");
		return -1;
	}

	if (got_output) 
	{
		if (ptCodecCtx->coded_frame->key_frame)	
					pkt.flags |= AV_PKT_FLAG_KEY;

		pkt.stream_index = ptVideoSt->index;
		ret = av_interleaved_write_frame(ptFormatCtx,&pkt);
	} 
	else {
		ret = 0;
	}

	return 0;
}

int CUdptrans::UdptransRelease()
{

	//Write file trailer
	av_write_trailer(ptFormatCtx);
	av_free(ptCodecCtx);
	
	for (unsigned int i = 0; i< ptFormatCtx->nb_streams;i++) 
	{
		av_freep(&(*ptFormatCtx->streams)->codec);
		av_freep(&ptFormatCtx->streams);
	}

	//Clean
	if (ptVideoSt){
		avcodec_close(ptVideoSt->codec);
		av_free(m_pYUVFrame);
		av_free(m_PicBuf);
	}
	avio_close(ptFormatCtx->pb);
	avformat_free_context(ptFormatCtx);

	return 0;
}

