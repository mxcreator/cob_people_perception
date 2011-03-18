/// @file PeopleDetector.cpp

#ifdef __LINUX__
#include "cob_people_detection/PeopleDetector.h"
#else
#include "cob_vision/cob_people_detection/common/include/cob_people_detection/PeopleDetector.h"
#endif

using namespace ipa_PeopleDetector;

PeopleDetector::PeopleDetector(void)
{	
	m_face_cascade = 0;
	m_range_cascade = 0;
}

unsigned long PeopleDetector::Init()
{	
	// Load Haar-Classifier for frontal face-, eyes- and body-detection
	m_face_cascade = (CvHaarClassifierCascade*)cvLoad("common/files/windows/haarcascades/haarcascade_frontalface_alt2.xml", 0, 0, 0 );	//"ConfigurationFiles/haarcascades/haarcascade_frontalface_alt2.xml", 0, 0, 0 );
	m_range_cascade = (CvHaarClassifierCascade*)cvLoad("common/files/windows/haarcascades/haarcascade_range.xml", 0, 0, 0 );

	// Create Memory
	m_storage = cvCreateMemStorage(0);

	return ipa_Utils::RET_OK;
}

PeopleDetector::~PeopleDetector(void)
{
	// Release Classifiers and memory
	cvReleaseHaarClassifierCascade(&m_face_cascade);
	cvReleaseHaarClassifierCascade(&m_range_cascade);
	cvReleaseMemStorage(&m_storage);
}

unsigned long PeopleDetector::DetectColorFaces(cv::Mat& img, std::vector<cv::Rect>& faceCoordinates)
{
	IplImage imgPtr = (IplImage)img;
	CvSeq* faces = cvHaarDetectObjects(
		&imgPtr,
		m_face_cascade, 
		m_storage, 
		m_faces_increase_search_scale, 
		m_faces_drop_groups, 
		CV_HAAR_DO_CANNY_PRUNING, 
		cvSize(m_faces_min_search_scale_x, m_faces_min_search_scale_y)
	);
	
	cv::Size parentSize;
	cv::Point roiOffset;
	for(int i=0; i<faces->total; i++)
	{
		cv::Rect* face = (cv::Rect*)cvGetSeqElem(faces, i);
		img.locateROI(parentSize, roiOffset);
		face->x += roiOffset.x;		// todo: check what happens if the original matrix is used without roi
		face->y += roiOffset.y;
		faceCoordinates.push_back(*face);
	}

	return ipa_Utils::RET_OK;
}

unsigned long PeopleDetector::DetectRangeFace(cv::Mat& img, std::vector<cv::Rect>& rangeFaceCoordinates)
{
	rangeFaceCoordinates.clear();

	IplImage imgPtr = (IplImage)img;
	CvSeq* rangeFaces = cvHaarDetectObjects
	(
		&imgPtr,
		m_range_cascade,
		m_storage,
		m_range_increase_search_scale,
		m_range_drop_groups,
		CV_HAAR_DO_CANNY_PRUNING,
		cvSize(m_range_min_search_scale_x, m_range_min_search_scale_y)
	);

	for(int i=0; i<rangeFaces->total; i++)
	{
		cv::Rect *rangeFace = (cv::Rect*)cvGetSeqElem(rangeFaces, i);
		rangeFaceCoordinates.push_back(*rangeFace);
	}

	return ipa_Utils::RET_OK;
}

unsigned long PeopleDetector::DetectFaces(cv::Mat& img, cv::Mat& rangeImg, std::vector<cv::Rect>& colorFaceCoordinates, std::vector<cv::Rect>& rangeFaceCoordinates)
{
	colorFaceCoordinates.clear();

	//######################################## Option1 ########################################
	DetectRangeFace(rangeImg, rangeFaceCoordinates);
	for(int i=0; i<(int)rangeFaceCoordinates.size(); i++)
	{
		cv::Rect rangeFace = rangeFaceCoordinates[i];
		
		rangeFace.y += rangeFace.height*0.1;

		cv::Mat areaImg = img(rangeFace);
// 		IplImage* areaImg = cvCloneImage(img);
// 		char* rowPtr = 0;
// 		for (int row=0; row<areaImg->height; row++ )
// 		{
// 			rowPtr = (char*)(areaImg->imageData + row*areaImg->widthStep);
// 			for (int col=0; col<areaImg->width; col++ )
// 			{
// 				if((col < rangeFace.x || col > (rangeFace.x + rangeFace.width)) || (row < rangeFace.y || row > (rangeFace.y + rangeFace.height)))
// 				{
// 					rowPtr[col*3] = 0;
// 					rowPtr[col*3+1] = 0;
// 					rowPtr[col*3+2] = 0;
// 				}		
// 			}
// 		}

		// Detect color Faces
		DetectColorFaces(areaImg, colorFaceCoordinates);
	}
	//######################################## /Option1 ########################################

	//######################################## Option2 ########################################
	/*DetectRangeFace(rangeImg, rangeFaceCoordinates);
	IplImage imgPtr = (IplImage)img;
	IplImage* areaImg = cvCloneImage(&imgPtr);
	char* rowPtr = 0;
	for (int row=0; row<areaImg->height; row++ )
	{
		rowPtr = (char*)(areaImg->imageData + row*areaImg->widthStep);
		for (int col=0; col<areaImg->width; col++ )
		{
			bool inrect=false;
			for(int i=0; i<(int)rangeFaceCoordinates->size(); i++)
			{
				cv::Rect rangeFace = rangeFaceCoordinates[i];
				
				rangeFace.y += rangeFace.height*0.1;

				if((col > rangeFace.x && col < (rangeFace.x + rangeFace.width)) && (row > rangeFace.y && row < (rangeFace.y + rangeFace.height)))
				{
					inrect=true;
					break;
				}
			}

			if(!inrect)
			{
				rowPtr[col*3] = 0;
				rowPtr[col*3+1] = 0;
				rowPtr[col*3+2] = 0;
			}
		}
	}

	// Detect color Faces
	cv::Mat areaImgMat(areaImg);
	DetectColorFaces(areaImgMat, colorFaceCoordinates);
	areaImgMat.release();
	cvReleaseImage(&areaImg);*/
	//######################################## /Option2 ########################################

	return ipa_Utils::RET_OK;
}

unsigned long PeopleDetector::AddFace(cv::Mat& img, cv::Rect& face, std::string id, std::vector<cv::Mat>& images, std::vector<std::string>& ids)
{
	//IplImage *resized_8U1 = cvCreateImage(cvSize(100, 100), 8, 1);
	cv::Mat resized_8U1(100, 100, CV_8UC1);
	ConvertAndResize(img, resized_8U1, face);
	
	// Save image
	images.push_back(resized_8U1);
	ids.push_back(id);

	return ipa_Utils::RET_OK;
}

unsigned long PeopleDetector::ConvertAndResize(cv::Mat& img, cv::Mat& resized, cv::Rect& face)
{
	cv::Mat temp;
	cv::cvtColor(img, temp, CV_BGR2GRAY);
	cv::Mat roi = temp(face);
	cv::resize(roi, resized, resized.size());

	return ipa_Utils::RET_OK;
}

unsigned long PeopleDetector::PCA(int* nEigens, std::vector<cv::Mat>& eigenVectors, cv::Mat& eigenValMat, cv::Mat& avgImage, std::vector<cv::Mat>& faceImages, cv::Mat& projectedTrainFaceMat)
{
	CvTermCriteria calcLimit;

	// Set the number of eigenvalues to use
	(*nEigens) = faceImages.size()-1;
	
	// Set the PCA termination criterion
	calcLimit = cvTermCriteria(CV_TERMCRIT_ITER, (*nEigens), 1);

	// Convert vector to array
	IplImage** faceImgArr = (IplImage**)cvAlloc((int)faceImages.size()*sizeof(IplImage*));
	for(int j=0; j<(int)faceImages.size(); j++)
	{
		IplImage temp = (IplImage)faceImages[j];
		faceImgArr[j] = &temp;
	}

	// Convert vector to array
	IplImage** eigenVectArr = (IplImage**)cvAlloc((int)eigenVectors.size()*sizeof(IplImage*));
	for(int j=0; j<(int)eigenVectors.size(); j++)
	{
		IplImage temp = (IplImage)eigenVectors[j];
		eigenVectArr[j] = &temp;
	}

	// Compute average image, eigenvalues, and eigenvectors
	IplImage avgImageIpl = (IplImage)avgImage;
	cvCalcEigenObjects(faceImages.size(), (void*)faceImgArr, (void*)eigenVectArr, CV_EIGOBJ_NO_CALLBACK, 0, 0, &calcLimit, &avgImageIpl, (float*)(eigenValMat.data));

	cv::normalize(eigenValMat,eigenValMat, 1, 0, CV_L1);	//, 0);		0=bug?

	// Project the training images onto the PCA subspace
	projectedTrainFaceMat.create(faceImages.size(), *nEigens, CV_32FC1);
	for(int i=0; i<(int)faceImages.size(); i++)
	{
		IplImage temp = (IplImage)faceImages[i];
		cvEigenDecomposite(&temp, *nEigens, eigenVectArr, 0, 0, &avgImageIpl, (float*)projectedTrainFaceMat.data + i* *nEigens);
	};

	cvFree(&faceImgArr);
	cvFree(&eigenVectArr);

	return ipa_Utils::RET_OK;
}

unsigned long PeopleDetector::RecognizeFace(ipa_SensorFusion::ColoredPointCloudPtr pc, std::vector<cv::Rect>& colorFaceCoordinates, int* nEigens, std::vector<cv::Mat>& eigenVectors, cv::Mat& avgImage, cv::Mat& projectedTrainFaceMat,
										std::vector<int>& index, int *threshold, int *threshold_FS, cv::Mat& eigenValMat)
{
	float* eigenVectorWeights = 0;

	cv::Mat resized_8U1(100, 100, CV_8UC1); // = cvCreateImage(cvSize(100, 100), 8, 1);

	eigenVectorWeights = (float *)cvAlloc(*nEigens*sizeof(float));

	// Convert vector to array
	IplImage** eigenVectArr = (IplImage**)cvAlloc((int)eigenVectors.size()*sizeof(IplImage*));
	for(int j=0; j<(int)eigenVectors.size(); j++)
	{
		IplImage temp = (IplImage)eigenVectors[j];
		eigenVectArr[j] = &temp;
	}
	
	for(int i=0; i<(int)colorFaceCoordinates.size(); i++)
	{
		cv::Rect face = colorFaceCoordinates[i];
		ConvertAndResize(pc->GetColorImage(), resized_8U1, face);

		IplImage avgImageIpl = (IplImage)avgImage;
		
		// Project the test image onto the PCA subspace
		IplImage resized_8U1Ipl = (IplImage)resized_8U1;
		cvEigenDecomposite(&resized_8U1Ipl, *nEigens, eigenVectArr, 0, 0, &avgImageIpl, eigenVectorWeights);

		cvFree(&eigenVectArr);

		// Calculate FaceSpace Distance
		cv::Mat srcReconstruction = cv::Mat::zeros(eigenVectors[0].size(), eigenVectors[0].type());
		for(int i=0; i<(int)eigenVectors.size(); i++) srcReconstruction += eigenVectorWeights[i]*eigenVectors[i];
		double distance = cv::norm((resized_8U1-avgImage), srcReconstruction, cv::NORM_L2);

		//######################################## Only for debugging and development ########################################
		/*std::cout.precision( 10 );
		std::cout << "FS_Distance: " << distance << std::endl;*/
		//######################################## /Only for debugging and development ########################################

		// -2=distance to face space is too high
		// -1=distance to face classes is too high
		if(distance > *threshold_FS)
		{
			// No face
			index.push_back(-2);
			//index.push_back(-2); why twice? apparently makes no sense.
		}
		else
		{
			int nearest;

			ClassifyFace(eigenVectorWeights, &nearest, nEigens, projectedTrainFaceMat, threshold, eigenValMat);
			if(nearest < 0)
			{
				// Face Unknown
				index.push_back(-1);
			}
			else
			{
				// Face known, it's number nearest
				index.push_back(nearest);
			}
		}
	}

	// Clear
	cvFree(&eigenVectorWeights);
	return ipa_Utils::RET_OK;
}

unsigned long PeopleDetector::ClassifyFace(float *eigenVectorWeights, int *nearest, int *nEigens, cv::Mat& projectedTrainFaceMat, int *threshold, cv::Mat& eigenValMat)
{
	double leastDistSq = DBL_MAX;

	for(int i=0; i<projectedTrainFaceMat.rows; i++)
	{
		double distance=0;
		
		for(int e=0; e<*nEigens; e++)
		{			
			float d = eigenVectorWeights[e] - ((float*)(projectedTrainFaceMat.data))[i * *nEigens + e];
			//distance += d*d;							//Euklid
			distance += d*d / ((float*)(eigenValMat.data))[e];	//Mahalanobis
		}
		//distance = sqrt(distance);

		//######################################## Only for debugging and development ########################################
		/*std::cout.precision( 10 );
		std::cout << "Distance_FC: " << distance << std::endl;*/
		//######################################## /Only for debugging and development ########################################

		if(distance < leastDistSq)
		{
			leastDistSq = distance;
			if(leastDistSq > *threshold)
			{
				*nearest = -1;
			}
			else
			{
				*nearest = i;
			}
		}
	}

	return ipa_Utils::RET_OK;
}

unsigned long PeopleDetector::CalculateFaceClasses(cv::Mat& projectedTrainFaceMat, std::vector<std::string>& id, int *nEigens)
{
	// Clone
	std::vector<std::string> id_tmp = id;
	id.clear();
	cv::Mat faces_tmp = projectedTrainFaceMat.clone();
	for (int i=0; i<((int)id_tmp.size() * *nEigens); i++)
	{
		((float*)(projectedTrainFaceMat.data))[i] = 0;
	}

	// Look for FaceClasses
	for(int i=0; i<(int)id_tmp.size(); i++)
	{
		std::string face_class = id_tmp[i];
		bool class_exists = false;
		
		for(int j=0; j<(int)id.size(); j++)
		{
			if(!id[j].compare(face_class))
			{
				class_exists = true;
			}
		}

		if(!class_exists)
		{
			id.push_back(face_class);
		}
	}

	cv::Size newSize(id.size(), *nEigens);
	projectedTrainFaceMat.create(newSize, faces_tmp.type());

	// Calculate FaceClasses
	for(int i=0; i<(int)id.size(); i++)
	{
		std::string face_class = id[i];
		
		for(int e=0; e<*nEigens; e++)
		{
			int count=0;
			for(int j=0;j<(int)id_tmp.size(); j++)
			{
				if(!(id_tmp[j].compare(face_class)))
				{
					((float*)(projectedTrainFaceMat.data))[i * *nEigens + e] += ((float*)(faces_tmp.data))[j * *nEigens + e];
					count++;
				}
			}
			((float*)(projectedTrainFaceMat.data))[i * *nEigens + e] /= (float)count;
		}
	}

	return ipa_Utils::RET_OK;
}