/*
 * This file is part of the as3kinect Project. http://www.as3kinect.org
 *
 * Copyright (c) 2010 individual as3kinect contributors. See the CONTRIB file
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
	
	import flash.display.Bitmap;
	import flash.display.BitmapData;
	import flash.events.EventDispatcher;
	import flash.geom.Rectangle;
	import flash.text.TextField;
	import flash.utils.ByteArray;
	
	import org.as3kinect.as3kinect;
	import org.as3kinect.as3kinectDepth;
	import org.as3kinect.as3kinectHand;
	import org.as3kinect.as3kinectSkeleton;
	import org.as3kinect.as3kinectSocket;
	import org.as3kinect.events.as3kinectSocketEvent;
	import org.as3kinect.events.as3kinectWrapperEvent;
	
	public class as3kinectWrapper extends EventDispatcher {

		private var _socket:as3kinectSocket;
		private var _data:ByteArray;
		private var _console:TextField;
		private var _debugging:Boolean = false;
		private var user_id:Number;
		
		public var depth:as3kinectDepth;
		public var skel:as3kinectSkeleton;
		public var hand:as3kinectHand;
		
		public function as3kinectWrapper() {
			depth = new as3kinectDepth();			
			skel = new as3kinectSkeleton();
			hand = new as3kinectHand();
			
			/* Init socket objects */
			_socket = as3kinectSocket.instance;
			_socket.connect(as3kinect.SERVER_IP, as3kinect.SOCKET_PORT);
			_socket.addEventListener(as3kinectSocketEvent.ONDATA, dataReceived);
			
			/* Init data out buffer */
			_data = new ByteArray();
		}

		/*
		 * dataReceived from socket (Protocol definition)
		 * Metadata comes in the first and second value of the data object
		 * first:
		 *	0 -> Camera data
		 * 			second:
		 *  			0 -> Depth ARGB received
		 *  			1 -> Video ARGB received
		 *  			2 -> Skeleton data received
		 *	1 -> Motor data
		 *	2 -> Microphone data
		 *	3 -> Server data
		 * 			second:
		 *  			0 -> Debug info received
		 *  			1 -> Got user
		 *  			2 -> Lost user
		 *  			3 -> Pose detected for user
		 *  			4 -> Calibrating user
		 *  			5 -> Calibration complete for user
		 *  			6 -> Calibration failed for user
		 *
		 */
		private function dataReceived(event:as3kinectSocketEvent):void{
			// Send ByteArray to position 0
			event.data.buffer.position = 0;
			switch (event.data.first) {
				case 0: //Camera
					switch (event.data.second) {
						case 0: //Depth received
							dispatchEvent(new as3kinectWrapperEvent(as3kinectWrapperEvent.ON_DEPTH, event.data.buffer));
							depth.busy = false;
						break;
						case 1: //Video received
							//dispatchEvent(new as3kinectWrapperEvent(as3kinectWrapperEvent.ON_DEPTH, event.data));
						break;
						case 2: //SKEL received
							skel.processSkeleton(event.data.buffer);
							dispatchEvent(new as3kinectWrapperEvent(as3kinectWrapperEvent.ON_SKEL, skel.skeletons));
							skel.busy = false;
						break;
						case 3: //HAND received
							hand.processHands(event.data.buffer);
							dispatchEvent(new as3kinectWrapperEvent(as3kinectWrapperEvent.ON_HAND, hand.hands));
							hand.busy = false;
							//Hand stuff here.
						break;
					}
				break;
				case 1: //Motor
				break;
				case 2: //Mic
				break;
				case 3: //Server
					switch (event.data.second) {
						case 0: //Debug received
							if(_debugging) _console.appendText(event.data.buffer.toString());
						break;
						case 1: //Got user
							user_id = event.data.buffer.readInt();
							if(_debugging) _console.appendText("Got user: " + user_id + "\n");
						break;
						case 2: //Lost user
							user_id = event.data.buffer.readInt();
							if(_debugging) _console.appendText("Lost user: " + user_id + "\n");
							skel.tracked_users.pop();
						break;
						case 3: //Pose detected
							user_id = event.data.buffer.readInt();
							if(_debugging) _console.appendText("Pose detected for user: " + user_id + "\n");
						break;
						case 4: //Calibrating
							user_id = event.data.buffer.readInt();
							if(_debugging) _console.appendText("Calibrating user: " + user_id + "\n");
						break;
						case 5: //Calibration complete
							user_id = event.data.buffer.readInt();
							if(_debugging) _console.appendText("Calibration complete for user: " + user_id + "\n");
							skel.tracked_users.push(user_id);
						break;
						case 6: //Calibration failed
							user_id = event.data.buffer.readInt();
							if(_debugging) _console.appendText("Calibration failed for user: " + user_id + "\n");
						break;
					}
				break;
			}
			// Clear ByteArray after used
			event.data.buffer.clear();
		}
		
		/*
		 * Enable log console on TextField
		 */
		public function set logConsole(txt:TextField):void{
			_debugging = true;
			_console = txt;
			_console.text = "=== Started console ===\n";
		}
	}
	
}
