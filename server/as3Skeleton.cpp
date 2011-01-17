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

#include "as3Skeleton.h"
#include <iostream>
using namespace std;

as3Skeleton::as3Skeleton() {
	this->size = 4 + 12 * 15;
	this->skel = new unsigned char[this->size];
	this->user_id = this->skel;
	this->head = this->skel+4+12*0;
	this->neck = this->skel+4+12*1;
	this->lshoulder =this->skel+4+12*2;
	this->lelbow =this->skel+4+12*3;
	this->lhand =this->skel+4+12*4;
	this->rshoulder =this->skel+4+12*5;
	this->relbow =this->skel+4+12*6;
	this->rhand =this->skel+4+12*7;
	this->torso =this->skel+4+12*8;
	this->lhip =this->skel+4+12*9;
	this->lknee =this->skel+4+12*10;
	this->lfoot =this->skel+4+12*11;
	this->rhip =this->skel+4+12*12;
	this->rknee =this->skel+4+12*13;
	this->rfoot =this->skel+4+12*14;
}

as3Skeleton::~as3Skeleton() {
	delete [] skel;
}