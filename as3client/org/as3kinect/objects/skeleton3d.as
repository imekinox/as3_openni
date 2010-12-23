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
 
package org.as3kinect.objects {
	
	import org.as3kinect.objects.point3d;
	import flash.utils.ByteArray;
	
	public class skeleton3d {
		public var user_id		:	uint;
		public var head			:	point3d;
		public var neck			:	point3d;
		public var l_shoulder	:	point3d;
		public var l_elbow		:	point3d;
		public var l_hand		:	point3d;
		public var r_shoulder	:	point3d;
		public var r_elbow		:	point3d;
		public var r_hand		:	point3d;
		public var torso		:	point3d;
		public var l_hip		:	point3d;
		public var l_knee		:	point3d;
		public var l_foot		:	point3d;
		public var r_hip		:	point3d;
		public var r_knee		:	point3d;
		public var r_foot		:	point3d;
		
		public function skeleton3d():void {
			this.user_id = 0;
			this.head = new point3d();
			this.neck = new  point3d();
			this.l_shoulder = new point3d();
			this.l_elbow = new  point3d();
			this.l_hand = new  point3d();
			this.r_shoulder = new point3d();
			this.r_elbow = new  point3d();
			this.r_hand = new  point3d();
			this.torso = new  point3d();
			this.l_hip = new  point3d();
			this.l_knee = new  point3d();
			this.l_foot = new  point3d();
			this.r_hip = new  point3d();
			this.r_knee = new  point3d();
			this.r_foot = new  point3d();
		}
		
		public function updateFromBytes(ba:ByteArray){
			this.user_id = ba.readInt();
			this.head.updatePoint(ba.readFloat(), ba.readFloat(), ba.readFloat());
			this.neck.updatePoint(ba.readFloat(), ba.readFloat(), ba.readFloat());
			this.l_shoulder.updatePoint(ba.readFloat(), ba.readFloat(), ba.readFloat());
			this.l_elbow.updatePoint(ba.readFloat(), ba.readFloat(), ba.readFloat());
			this.l_hand.updatePoint(ba.readFloat(), ba.readFloat(), ba.readFloat());
			this.r_shoulder.updatePoint(ba.readFloat(), ba.readFloat(), ba.readFloat());
			this.r_elbow.updatePoint(ba.readFloat(), ba.readFloat(), ba.readFloat());
			this.r_hand.updatePoint(ba.readFloat(), ba.readFloat(), ba.readFloat());
			this.torso.updatePoint(ba.readFloat(), ba.readFloat(), ba.readFloat());
			this.l_hip.updatePoint(ba.readFloat(), ba.readFloat(), ba.readFloat());
			this.l_knee.updatePoint(ba.readFloat(), ba.readFloat(), ba.readFloat());
			this.l_foot.updatePoint(ba.readFloat(), ba.readFloat(), ba.readFloat());
			this.r_hip.updatePoint(ba.readFloat(), ba.readFloat(), ba.readFloat());
			this.r_knee.updatePoint(ba.readFloat(), ba.readFloat(), ba.readFloat());
			this.r_foot.updatePoint(ba.readFloat(), ba.readFloat(), ba.readFloat());
		}
	}
}
