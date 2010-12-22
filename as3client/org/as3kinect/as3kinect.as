/*
 * This file is part of the as3server Project. http://www.as3server.org
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

 package org.as3kinect {
	 
 	import flash.utils.ByteArray;
	
	public class as3kinect {

		public static const SUCCESS:int = 0;
		public static const ERROR:int = -1;

		public static const SERVER_IP:String = "localhost";
		public static const SOCKET_PORT:int = 6001;

		public static const CAMERA_ID:int = 0;
		public static const MOTOR_ID:int = 1;
		public static const MIC_ID:int = 2;
		
		public static const GET_DEPTH:int = 0;
		public static const GET_RGB:int = 1;
		public static const GET_SKEL:int = 2;
		
		public static const IMG_WIDTH:int = 640;
		public static const IMG_HEIGHT:int = 480;

		public static const RAW_IMG_SIZE:int = IMG_WIDTH * IMG_HEIGHT * 4;
		public static const DATA_IN_SIZE:int = 3 * 2 + 3 * 8;
		public static const COMMAND_SIZE:int = 6;
	}
}
