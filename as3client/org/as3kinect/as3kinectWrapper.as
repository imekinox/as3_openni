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
	
	import org.as3kinect.as3kinect;
	import org.as3kinect.as3kinectSocket;
	import org.as3kinect.objects.skeleton3d;
	import org.as3kinect.events.as3kinectSocketEvent;
	import org.as3kinect.events.as3kinectWrapperEvent;
	
	import flash.utils.ByteArray;
	import flash.display.BitmapData;
	import flash.display.Bitmap;
	import flash.geom.Rectangle;
	import flash.events.EventDispatcher;
	import flash.text.TextField;
	
	public class as3kinectWrapper extends EventDispatcher {

		private var _socket:as3kinectSocket;
		private var _data:ByteArray;
		private var _depth_buff_busy:Boolean = false;
		private var _skel_buff_busy:Boolean = false;
		private var _console:TextField;
		private var _debugging:Boolean = false;
		private var user_id:Number;
		private var _skel:Object;
		private var _tracked_users:Array;

		public function as3kinectWrapper() {
			/* Init socket objects */
			_socket = as3kinectSocket.instance;
			_socket.connect(as3kinect.SERVER_IP, as3kinect.SOCKET_PORT);
			_socket.addEventListener(as3kinectSocketEvent.ONDATA, dataReceived);
			
			/* Init data out buffer */
			_data = new ByteArray();
			_skel = new skeleton3d();
			_tracked_users = new Array();
		}
		
		/*
		 * Draw ARGB from ByteArray to BitmapData object
		 */
		public function byteArrayToBitmapData(bytes:ByteArray, _canvas:BitmapData):void{
			_canvas.lock();
			_canvas.setPixels(new Rectangle(0,0, as3kinect.IMG_WIDTH, as3kinect.IMG_HEIGHT), bytes);
			_canvas.unlock();
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
							_depth_buff_busy = false;
						break;
						case 1: //Video received
							//dispatchEvent(new as3kinectWrapperEvent(as3kinectWrapperEvent.ON_DEPTH, event.data));
						break;
						case 2: //SKEL received
							as3kinect.byteToSkel(_skel, event.data.buffer);
							dispatchEvent(new as3kinectWrapperEvent(as3kinectWrapperEvent.ON_SKEL, _skel));
							_skel_buff_busy = false;
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
							_tracked_users.pop();
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
							_tracked_users.push(user_id);
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
		 *	Client -> Server communication protocol:
		 *	first byte:
		 * 		0 -> Camera command
		 * 			second byte:
		 * 				0 -> Get depth ARGB
		 * 				0 -> Get video ARGB
 		 * 				0 -> Get skeleton data
		 */

		/*
		 * Tell server to send the latest depth frame
		 * Note: We should lock the command while we are waiting for the data to avoid lag
		 */
		public function getDepthBuffer():void {
			if(!_depth_buff_busy){
				_depth_buff_busy = true;
				_data.clear();
				_data.writeByte(as3kinect.CAMERA_ID);
				_data.writeByte(as3kinect.GET_DEPTH);
				_data.writeInt(0);
				if(_socket.sendCommand(_data) != as3kinect.SUCCESS){
					throw new Error('Data was not complete');
				}
			}
		}
		
		/*
		 * Tell server to send the latest skeleton data
		 * Note: We should lock the command while we are waiting for the data to avoid lag
		 */
		public function getSkeleton():void {
			if(!_skel_buff_busy && _tracked_users.length > 0){
				_skel_buff_busy = true;
				_data.clear();
				_data.writeByte(as3kinect.CAMERA_ID);
				_data.writeByte(as3kinect.GET_SKEL);
				_data.writeInt(0);
				if(_socket.sendCommand(_data) != as3kinect.SUCCESS){
					throw new Error('Data was not complete');
				}
			}
		}
		
		/*
		 * Enable log console on TextField
		 */
		public function set logConsole(txt:TextField){
			_debugging = true;
			_console = txt;
			_console.text = "=== Started console ===\n";
		}
	}
	
}
