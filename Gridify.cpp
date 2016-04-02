#include "Gridify.h"
#include "opencv2/opencv.hpp"
#include "opencv2/nonfree/features2d.hpp"

#pragma comment(lib, "opencv_core2412.lib")
#pragma comment(lib, "opencv_features2d2412.lib")
#pragma comment(lib, "opencv_imgproc2412.lib")
#pragma comment(lib, "opencv_nonfree2412.lib")

using namespace cv;

static PF_Err ParamsSetup (PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)
{
	PF_Err		err		= PF_Err_NONE;
	PF_ParamDef	def;	

	//AEFX_CLR_STRUCT(def);
	//def.flags = PF_ParamFlag_START_COLLAPSED;
	//PF_ADD_TOPIC( "featurepoint detection", SURFtopic_DISK_ID);
	
	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Threshold",
		0, 		// MIN
		90000,	// MAX
		0,		// MIN
		90000, 	// MAX
		500,	// Default 500 - 4000
		Threshold_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Octaves",
		1, 		// MIN
		16,		// MAX
		1,		// MIN
		16, 	// MAX
		2,		// Default
		Octaves_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Layers",
		1, 		// MIN
		16,		// MAX
		1,		// MIN
		16, 	// MAX
		4,		// Default
		Layers_DISK_ID);

	//AEFX_CLR_STRUCT(def);
	//PF_END_TOPIC(SURFtopic_END_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Grid X",
		1, 		// MIN
		64,		// MAX
		1,		// MIN
		64, 	// MAX
		12,		// Default
		GridX_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Grid Y",
		1, 		// MIN
		64,		// MAX
		1,		// MIN
		64, 	// MAX
		12,		// Default
		GridY_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_SLIDER("Separation",
		0, 		// MIN
		64,		// MAX
		0,		// MIN
		64, 	// MAX
		2,		// Default
		Separation_DISK_ID);

	out_data->num_params = SKELETON_NUM_PARAMS;
	return err;
}

static PF_Err Render( PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)
{
	PF_Err				err		= PF_Err_NONE;
	AEGP_SuiteHandler	suites(in_data->pica_basicP);
	PF_EffectWorld *input = &params[0]->u.ld;
	PF_COPY(input, output, NULL, NULL);

	A_long width = output->width;
	A_long height = output->height;
	A_long outWidth = output->rowbytes / sizeof(PF_Pixel);
	PF_Pixel *AEFrameData = (PF_Pixel *)output->data;

	/* PF_Pixel > CvMat */
	Mat src = Mat(Size(outWidth, height), CV_8UC4);		// 8bit BGRA
	for (int y = 0; y < src.rows; ++y) {
		for (int x = 0; x < src.cols; ++x) {
			src.data[y * src.step + x * src.elemSize() + 0] = AEFrameData[x + y * outWidth].blue;
			src.data[y * src.step + x * src.elemSize() + 1] = AEFrameData[x + y * outWidth].green;
			src.data[y * src.step + x * src.elemSize() + 2] = AEFrameData[x + y * outWidth].red;
			src.data[y * src.step + x * src.elemSize() + 3] = AEFrameData[x + y * outWidth].alpha;
		}
	}
	
	/* OpenCV SURF */
	Mat gray_img;
	cvtColor(src, gray_img, CV_BGR2GRAY);
	normalize(gray_img, gray_img, 0, 255, cv::NORM_MINMAX);

	vector<KeyPoint> keypoints;
	vector<KeyPoint>::iterator it;

	SurfFeatureDetector surf(params[Param_Threshold]->u.sd.value, params[Param_Octaves]->u.sd.value, params[Param_Layers]->u.sd.value);
	surf.detect(gray_img, keypoints);
	Vec4b srcColor = Vec4d(0, 0, 0, 0);

	int cvwidth = src.size().width;
	int cvheight = src.size().height;
	int grid_x = params[Param_GridX]->u.sd.value;
	int grid_y = params[Param_GridY]->u.sd.value;
	int step_x = (int)(cvwidth / grid_x);
	int step_y = (int)(cvheight / grid_y); 
	int separation = params[Param_Separation]->u.sd.value + 1;

	/* gridify */
	Mat canvas = cv::Mat(src.size(), CV_8UC4);
	canvas = cv::Scalar(0, 0, 0, 0);

	for (it = keypoints.begin(); it != keypoints.end(); ++it) {
		Point2f point = it->pt;
		int size = (int)(it->size / 1.2);

		srcColor = src.at<Vec4b>(point);
		Scalar color = Scalar(srcColor[0], srcColor[1], srcColor[2], 255);

		Point gridfy_pt;
		Point gridfy_pt2;
		for (int step = 0; step < step_x; ++step) {
			if(point.x >= grid_x * step && point.x <= grid_x * (step+1)){
				gridfy_pt.x = grid_x * step;
				gridfy_pt2.x = grid_x * step + ((size / grid_x) * grid_x) - separation;
				break;
			}
		}
		for (int step = 0; step < step_y; ++step) {
			if(point.y >= grid_y * step && point.y <= grid_y * (step+1)){
				gridfy_pt.y = grid_y * step;
				gridfy_pt2.y = grid_y * step + ((size / grid_y) * grid_y) - separation;
				break;
			}
		}

		if(gridfy_pt.x < gridfy_pt2.x && gridfy_pt2.x <= src.size().width && gridfy_pt2.y <= src.size().height){
			if(canvas.at<Vec4b>(gridfy_pt)[3] == 0 && canvas.at<Vec4b>(gridfy_pt2)[3] == 0 && canvas.at<Vec4b>(Point(gridfy_pt2.x, gridfy_pt.y))[3] == 0 && canvas.at<Vec4b>(Point(gridfy_pt.x, gridfy_pt2.y))[3] == 0){
				rectangle(canvas, gridfy_pt, gridfy_pt2, color, CV_FILLED);
			}
		}
	}


	/* CvMat > PF_Pixel */
	for (int y = 0; y < canvas.rows; ++y) {
		for (int x = 0; x < canvas.cols; ++x) {
			AEFrameData[x + y * outWidth].blue	=	canvas.data[y * canvas.step + x * canvas.elemSize() + 0];
			AEFrameData[x + y * outWidth].green	=	canvas.data[y * canvas.step + x * canvas.elemSize() + 1];
			AEFrameData[x + y * outWidth].red	=	canvas.data[y * canvas.step + x * canvas.elemSize() + 2];
			AEFrameData[x + y * outWidth].alpha	=	canvas.data[y * canvas.step + x * canvas.elemSize() + 3];
		}
	}

	return err;
}

static PF_Err About(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);

	suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg,
		"%s v%d.%d\r%s",
		STR(StrID_Name),
		MAJOR_VERSION,
		MINOR_VERSION,
		STR(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err GlobalSetup(PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output)
{
	out_data->my_version = PF_VERSION(MAJOR_VERSION,
		MINOR_VERSION,
		BUG_VERSION,
		STAGE_VERSION,
		BUILD_VERSION);

	out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE;

	return PF_Err_NONE;
}

DllExport PF_Err EntryPointFunc(PF_Cmd cmd, PF_InData *in_data, PF_OutData *out_data, PF_ParamDef *params[], PF_LayerDef *output, void *extra)
{
	PF_Err err = PF_Err_NONE;
	
	try {
		switch (cmd) {
			case PF_Cmd_ABOUT:

				err = About(in_data,
							out_data,
							params,
							output);
				break;
				
			case PF_Cmd_GLOBAL_SETUP:

				err = GlobalSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_PARAMS_SETUP:

				err = ParamsSetup(	in_data,
									out_data,
									params,
									output);
				break;
				
			case PF_Cmd_RENDER:

				err = Render(	in_data,
								out_data,
								params,
								output);
				break;
		}
	}
	catch(PF_Err &thrown_err){
		err = thrown_err;
	}
	return err;
}

