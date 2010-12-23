/*
 * This file is part of the as3kinect Project. http://www.as3kinect.org
 *
 * Copyright (c) 2010 individual as3server contributors. See the CONTRIB file
 * for details.
 *
 * This code is licensed to you under the terms of the Apache License, version
 * 2.0, or, at your option, the terms of the GNU General Public License,
 * version 2.0. See the APACHE20 and GPL2 files for the text of the licenses,
 * or the following URLs:
 * http://www.apache.org/licenses/LICENSE-2.0
 * http://www.gnu.org/licenses/gpl-2.0.txt
 *
 * If you redistribute this file in source form, modified or unmodified, you
 * may:
 *   1) Leave this header intact and distribute it under the same terms,
 *      accompanying it with the APACHE20 and GPL20 files, or
 *   2) Delete the Apache 2.0 clause and accompany it with the GPL2 file, or
 *   3) Delete the GPL v2 clause and accompany it with the APACHE20 file
 * In all cases you must keep the copyright notice intact and include a copy
 * of the CONTRIB file.
 *
 * Binary distributions must follow the binary distribution requirements of
 * either License.
 */

#include <XnOpenNI.h>
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>
#include <XnFPSCalculator.h>
#include <pthread\pthread.h>

#include "as3Network.h"


//---------------------------------------------------------------------------
// Globals
//---------------------------------------------------------------------------
xn::Context g_Context;
xn::DepthGenerator g_DepthGenerator;
xn::UserGenerator g_UserGenerator;

XnBool g_bNeedPose = FALSE;
XnChar g_strPose[20] = "";
XnBool g_bDrawBackground = TRUE;
XnBool g_bDrawPixels = TRUE;
XnBool g_bDrawSkeleton = TRUE;
XnBool g_bPrintID = TRUE;
XnBool g_bPrintState = TRUE;

#define GL_WIN_SIZE_X 720
#define GL_WIN_SIZE_Y 480

XnBool g_bPause = false;
XnBool g_bRecord = false;

XnBool g_bQuit = false;

as3Network server;
unsigned char img_buffer[4*640*480];
unsigned char img_buffer_meta[4*640*480+6];
unsigned char skel[4 + 12 * 15];

#define MAX_DEPTH 10000
float g_pDepthHist[MAX_DEPTH];

pthread_t server_thread;
int _die = 0;
int _connected = 0;
XnFPSData xnFPS;

XnFloat Colors[][3] =
{
	{0,1,1},
	{0,0,1},
	{0,1,0},
	{1,1,0},
	{1,0,0},
	{1,.5,0},
	{.5,1,0},
	{0,.5,1},
	{.5,0,1},
	{1,1,.5},
	{1,1,1}
};

XnUInt32 nColors = 10;

unsigned char *head = new unsigned char[12];
unsigned char *neck = new unsigned char[12];
unsigned char *lshoulder = new unsigned char[12];
unsigned char *lelbow = new unsigned char[12];
unsigned char *lhand = new unsigned char[12];
unsigned char *rshoulder = new unsigned char[12];
unsigned char *relbow = new unsigned char[12];
unsigned char *rhand = new unsigned char[12];
unsigned char *torso = new unsigned char[12];
unsigned char *lhip = new unsigned char[12];
unsigned char *lknee = new unsigned char[12];
unsigned char *lfoot = new unsigned char[12];
unsigned char *rhip = new unsigned char[12];
unsigned char *rknee = new unsigned char[12];
unsigned char *rfoot = new unsigned char[12];

//---------------------------------------------------------------------------
// Code
//---------------------------------------------------------------------------

void CleanupExit()
{
	g_Context.Shutdown();
	server.close_connection();
	_die = 1;
	exit (1);
}

void getJointPosition(XnUserID player, XnSkeletonJoint eJoint1, unsigned char * dest)
{
	if (!g_UserGenerator.GetSkeletonCap().IsTracking(player))
	{
		printf("not tracked!\n");
		return;
	}
	XnSkeletonJointPosition joint1;
	g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player, eJoint1, joint1);

	if (joint1.fConfidence < 0.5)
	{
		return;
	}
	XnPoint3D pt[1];
	pt[0] = joint1.position;
	g_DepthGenerator.ConvertRealWorldToProjective(1, pt, pt);
	float _x, _y, _z;
	_x = pt[0].X;
	_y = pt[0].Y;
	_z = pt[0].Z;
	memcpy(dest, &_x, 4);
	memcpy(dest+4, &_y, 4);
	memcpy(dest+8, &_z, 4);
}

void getDepthMap(unsigned char* img_buffer)
{
	xn::SceneMetaData smd;
	xn::DepthMetaData dmd;
	g_DepthGenerator.GetMetaData(dmd);

	//printf("Frame %d Middle point is: %u. FPS: %f\n", dmd.FrameID(), dmd(dmd.XRes() / 2, dmd.YRes() / 2), xnFPSCalc(&xnFPS));

	if (!g_bPause)
	{
		g_Context.WaitAndUpdateAll();
	}

	g_DepthGenerator.GetMetaData(dmd);
	g_UserGenerator.GetUserPixels(0, smd);
	unsigned int nValue = 0;
	unsigned int nHistValue = 0;
	unsigned int nIndex = 0;
	unsigned int nX = 0;
	unsigned int nY = 0;
	unsigned int nNumberOfPoints = 0;
	XnUInt16 g_nXRes = dmd.XRes();
	XnUInt16 g_nYRes = dmd.YRes();

	const XnDepthPixel* pDepth = dmd.Data();
	const XnLabel* pLabels = smd.Data();

	// Calculate the accumulative histogram
	memset(g_pDepthHist, 0, MAX_DEPTH*sizeof(float));
	for (nY=0; nY<g_nYRes; nY++)
	{
		for (nX=0; nX<g_nXRes; nX++)
		{
			nValue = *pDepth;

			if (nValue != 0)
			{
				g_pDepthHist[nValue]++;
				nNumberOfPoints++;
			}

			pDepth++;
		}
	}

	for (nIndex=1; nIndex<MAX_DEPTH; nIndex++)
	{
		g_pDepthHist[nIndex] += g_pDepthHist[nIndex-1];
	}

	if (nNumberOfPoints)
	{
		for (nIndex=1; nIndex<MAX_DEPTH; nIndex++)
		{
			g_pDepthHist[nIndex] = (unsigned int)(256 * (1.0f - (g_pDepthHist[nIndex] / nNumberOfPoints)));
		}
	}
	pDepth = dmd.Data();
	if (g_bDrawPixels)
	{
		XnUInt32 nIndex = 0;
		for (nY=0; nY<g_nYRes; nY++)
		{
			for (nX=0; nX < g_nXRes; nX++, nIndex++)
			{
				img_buffer[0] = 0;
				img_buffer[1] = 0;
				img_buffer[2] = 0;
				img_buffer[3] = 0xFF;
				if (g_bDrawBackground || *pLabels != 0)
				{
					nValue = *pDepth;
					XnLabel label = *pLabels;
					XnUInt32 nColorID = label % nColors;
					if (label == 0)
					{
						nColorID = nColors;
					}

					if (nValue != 0)
					{
						nHistValue = g_pDepthHist[nValue];
						img_buffer[0] = nHistValue * Colors[nColorID][0]; 
						img_buffer[1] = nHistValue * Colors[nColorID][1];
						img_buffer[2] = nHistValue * Colors[nColorID][2];
						img_buffer[3] = 0xFF;
					}
				}
				pDepth++;
				pLabels++;
				img_buffer+=4;
			}
		}
	}

	XnUserID aUsers[15];
	XnUInt16 nUsers = 15;
	g_UserGenerator.GetUsers(aUsers, nUsers);
	for (int i = 0; i < nUsers; ++i)
	{
		if (g_bDrawSkeleton && g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i]))
		{
			getJointPosition(aUsers[i], XN_SKEL_HEAD, head);
			getJointPosition(aUsers[i], XN_SKEL_NECK, neck);

			getJointPosition(aUsers[i], XN_SKEL_LEFT_SHOULDER, lshoulder);
			getJointPosition(aUsers[i], XN_SKEL_LEFT_ELBOW, lelbow);
			getJointPosition(aUsers[i], XN_SKEL_LEFT_HAND, lhand);

			getJointPosition(aUsers[i], XN_SKEL_RIGHT_SHOULDER, rshoulder);
			getJointPosition(aUsers[i], XN_SKEL_RIGHT_ELBOW, relbow);
			getJointPosition(aUsers[i], XN_SKEL_RIGHT_HAND, rhand);

			getJointPosition(aUsers[i], XN_SKEL_TORSO, torso);

			getJointPosition(aUsers[i], XN_SKEL_LEFT_HIP, lhip);
			getJointPosition(aUsers[i], XN_SKEL_LEFT_KNEE, lknee);
			getJointPosition(aUsers[i], XN_SKEL_LEFT_FOOT, lfoot);

			getJointPosition(aUsers[i], XN_SKEL_RIGHT_HIP, rhip);
			getJointPosition(aUsers[i], XN_SKEL_RIGHT_KNEE, rknee);
			getJointPosition(aUsers[i], XN_SKEL_RIGHT_FOOT, rfoot);
			
			int user_id = aUsers[i];
			memcpy(skel, &user_id, 4);
			memcpy(skel+4+12*0, head, 12);
			memcpy(skel+4+12*1, neck, 12);
			memcpy(skel+4+12*2, lshoulder, 12);
			memcpy(skel+4+12*3, lelbow, 12);
			memcpy(skel+4+12*4, lhand, 12);
			memcpy(skel+4+12*5, rshoulder, 12);
			memcpy(skel+4+12*6, relbow, 12);
			memcpy(skel+4+12*7, rhand, 12);
			memcpy(skel+4+12*8, torso, 12);
			memcpy(skel+4+12*9, lhip, 12);
			memcpy(skel+4+12*10, lknee, 12);
			memcpy(skel+4+12*11, lfoot, 12);
			memcpy(skel+4+12*12, rhip, 12);
			memcpy(skel+4+12*13, rknee, 12);
			memcpy(skel+4+12*14, rfoot, 12);
		}
	}
}

// Callback: New user was detected
void XN_CALLBACK_TYPE User_NewUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
	server.sendMessage(3,1,nId);
	// New user found
	if (g_bNeedPose)
	{
		g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
	}
	else
	{
		g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
	}
}
// Callback: An existing user was lost
void XN_CALLBACK_TYPE User_LostUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
	server.sendMessage(3,2,nId);
}
// Callback: Detected a pose
void XN_CALLBACK_TYPE UserPose_PoseDetected(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie)
{
	server.sendMessage(3,3,nId);
	g_UserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
	g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
}
// Callback: Started calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie)
{
	server.sendMessage(3,4,nId);
}
// Callback: Finished calibration
void XN_CALLBACK_TYPE UserCalibration_CalibrationEnd(xn::SkeletonCapability& capability, XnUserID nId, XnBool bSuccess, void* pCookie)
{
	if (bSuccess)
	{
		// Calibration succeeded
		server.sendMessage(3,5,nId);
		g_UserGenerator.GetSkeletonCap().StartTracking(nId);
	}
	else
	{
		// Calibration failed
		server.sendMessage(3,6,nId);
		if (g_bNeedPose)
		{
			g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
		}
		else
		{
			g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
		}
	}
}

void *server_data(void *arg) {
	unsigned char buff[1024];
	while(_connected){
		int n = server.getData(buff, 1024);
		if(n == 6){
			switch(buff[0]){
				case 0: //CAMERA
					switch(buff[1]){
						case 0: //GET DEPTH
							server.sendMessage(0,0,img_buffer, sizeof(img_buffer));
						break;
						case 1: //GET RGB
						break;
						case 2: //GET SKEL
							server.sendMessage(0,2,skel, sizeof(skel));
						break;
					}
				break;
				case 1: //MOTOR
				break;
				case 2: //MIC
				break;
			}
		}
		else {
			printf("got bad command: %d\n", n);
			server.close_connection();
			_connected = 0;
		}
	}
	return NULL;
}

void server_connected() {
	_connected = 1;
	if (pthread_create(&server_thread, NULL, &server_data, NULL)) {
		fprintf(stderr, "Error on pthread_create() for SERVER\n");
	}
	server.sendMessage("Server connected\n");
}

#define SAMPLE_XML_PATH "SamplesConfig.xml"

#define CHECK_RC(nRetVal, what)										\
	if (nRetVal != XN_STATUS_OK)									\
	{																\
		printf("%s failed: %s\n", what, xnGetStatusString(nRetVal));\
		return nRetVal;												\
	}


int main(int argc, char **argv)
{
	XnStatus nRetVal = XN_STATUS_OK;

	if (argc > 1)
	{
		nRetVal = g_Context.Init();
		CHECK_RC(nRetVal, "Init");
		nRetVal = g_Context.OpenFileRecording(argv[1]);
		if (nRetVal != XN_STATUS_OK)
		{
			printf("Can't open recording %s: %s\n", argv[1], xnGetStatusString(nRetVal));
			return 1;
		}
	}
	else
	{
		nRetVal = g_Context.InitFromXmlFile(SAMPLE_XML_PATH);
		CHECK_RC(nRetVal, "InitFromXml");
	}

	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_DEPTH, g_DepthGenerator);
	CHECK_RC(nRetVal, "Find depth generator");

	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_USER, g_UserGenerator);
	if (nRetVal != XN_STATUS_OK)
	{
		nRetVal = g_UserGenerator.Create(g_Context);
		CHECK_RC(nRetVal, "Find user generator");
	}

	XnCallbackHandle hUserCallbacks, hCalibrationCallbacks, hPoseCallbacks;
	if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON))
	{
		printf("Supplied user generator doesn't support skeleton\n");
		return 1;
	}
	g_UserGenerator.RegisterUserCallbacks(User_NewUser, User_LostUser, NULL, hUserCallbacks);
	g_UserGenerator.GetSkeletonCap().RegisterCalibrationCallbacks(UserCalibration_CalibrationStart, UserCalibration_CalibrationEnd, NULL, hCalibrationCallbacks);

	if (g_UserGenerator.GetSkeletonCap().NeedPoseForCalibration())
	{
		g_bNeedPose = TRUE;
		if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION))
		{
			printf("Pose required, but not supported\n");
			return 1;
		}
		g_UserGenerator.GetPoseDetectionCap().RegisterToPoseCallbacks(UserPose_PoseDetected, NULL, NULL, hPoseCallbacks);
		g_UserGenerator.GetSkeletonCap().GetCalibrationPose(g_strPose);
	}

	g_UserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);

	nRetVal = g_Context.StartGeneratingAll();
	CHECK_RC(nRetVal, "StartGenerating");

	nRetVal = xnFPSInit(&xnFPS, 180);
	CHECK_RC(nRetVal, "FPS Init");

	server = as3Network();
	server.init();
	server.onConnect(&server_connected);


	while(!_die){
		xnFPSMarkFrame(&xnFPS);
		getDepthMap(img_buffer);
	}
}
