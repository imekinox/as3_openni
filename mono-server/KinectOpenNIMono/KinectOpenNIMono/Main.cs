using System;
using System.Threading;
using System.Net.Sockets;
using System.Net;
using System.IO;
using System.Text;
using System.Collections.Generic;

namespace KinectOpenNIMono
{
	
	class MainClass
	{
		private readonly string SAMPLE_XML_FILE = @"../../Sample-Tracking.xml";

		public static ManualResetEvent allDone = new ManualResetEvent (false);

		private xn.Context context;
		private Thread readerThread;
		private bool shouldRun;
		private xnv.SessionManager sessionManager;
		private xnv.PointControl pointControl;
		private xnv.HandPointContext primaryHand;
		
		private List<xnv.HandPointContext> handPoints;
		
		public static void Main (string[] args)
		{
			MainClass main = new MainClass ();
			main.SetupKinect ();
			main.SetupServer ();
		}

		private void SetupKinect ()
		{
			this.context = new xn.Context (SAMPLE_XML_FILE);
			this.sessionManager = new xnv.SessionManager (this.context, "Wave", "RaiseHand");
			
			this.pointControl = new xnv.PointControl ();
			
			this.sessionManager.SessionStart += new xnv.SessionManager.SessionStartHandler (sessionManager_SessionStart);
			
			this.primaryHand = new xnv.HandPointContext ();
			this.handPoints = new List<xnv.HandPointContext> ();
			
			this.pointControl.PointDestroy += new xnv.PointControl.PointDestroyHandler (pointControl_PointDestroy);
			this.pointControl.PointUpdate += new xnv.PointControl.PointUpdateHandler (pointControl_PointUpdate);
			this.pointControl.PointCreate += new xnv.PointControl.PointCreateHandler (pointControl_PointCreate);
			
			this.pointControl.PrimaryPointCreate += new xnv.PointControl.PrimaryPointCreateHandler (pointControl_PrimaryPointCreate);
			this.pointControl.PrimaryPointReplace += new xnv.PointControl.PrimaryPointReplaceHandler (pointControl_PrimaryPointReplace);
			this.pointControl.PrimaryPointDestroy += new xnv.PointControl.PrimaryPointDestroyHandler (pointControl_PrimaryPointDestroy);
			
			this.sessionManager.AddListener (this.pointControl);
			
			this.shouldRun = true;
			this.readerThread = new Thread (ReaderThread);
			this.readerThread.Start ();
		}

		private void ReaderThread ()
		{
			while (this.shouldRun) {
				try {
					this.context.WaitAndUpdateAll ();
					this.sessionManager.Update (this.context);
				} catch (System.Exception) {
					
				}
			}
		}

		void sessionManager_SessionStart (ref xn.Point3D position)
		{
			
		}
		
		void pointControl_PointCreate (ref xnv.HandPointContext context)
		{
			handPoints.Add (context);
			
		}
		
		void pointControl_PointUpdate (ref xnv.HandPointContext context)
		{
			string sendData = 
					"Pos(" + context.ptPosition.X.ToString () + ", " 
						+ context.ptPosition.Y.ToString () + ", " 
						+ context.ptPosition.Z.ToString () + ") "
						+ "nID: " + context.nID.ToString () 
						+ " nUserID: " + context.nUserID.ToString ();
				
				
				Console.Out.WriteLine ("PointUpdate: " + sendData);
		}
		
		void pointControl_PointDestroy (uint id)
		{
			foreach (xnv.HandPointContext hpc in handPoints)
			{
				if (hpc.nID == id) {
					handPoints.Remove (hpc);
				}
			}
		}
		
		void pointControl_PrimaryPointCreate (ref xnv.HandPointContext context, ref xn.Point3D pnt)
		{
			primaryHand = context;
		}

		void pointControl_PrimaryPointReplace (uint nID, ref xnv.HandPointContext context)
		{
			primaryHand = context;
		}

		void pointControl_PrimaryPointDestroy (uint id)
		{
			primaryHand = new xnv.HandPointContext ();
		}

		
		void processClientRequest (byte[] data, Socket clientSocket)
		{
			//byte cameraID = data[0];
			byte requestNum = data[1];
			
			switch (requestNum) {
			case 0:
				//get depth buffer
				break;
			case 1:
				//get rgb buffer
				break;
			case 2:
				//get skeleton
				break;
			case 3:
				//get hands
				uint numSent = 0;
				
				foreach (xnv.HandPointContext hpc in handPoints)
				{
					MemoryStream ms = new MemoryStream ();
					BinaryWriter bw = new BinaryWriter (ms);
					bw.Write ((byte)0);
					bw.Write ((byte)3);
					bw.Write (hpc.nID);
					bw.Write (hpc.nUserID);
					bw.Write (hpc.Equals (primaryHand));
					bw.Write (hpc.ptPosition.X);
					bw.Write (hpc.ptPosition.Y);
					bw.Write (hpc.ptPosition.Z);
					
					Send (clientSocket, ms.ToArray ());
					numSent++;
				}
				
				if (numSent == 0) {
					
					byte[] ba = new byte[6];
					
					//camera data
					ba[0] = 0;
					//hand response
					ba[1] = 3;
					//first int, tell it nID = 0
					ba[2] = 0;
					ba[3] = 0;
					ba[4] = 0; 
					ba[5] = 0;
					
					Send (clientSocket, ba);
				}
				break;
			default:
				break;
			}
			
		}

		#region Async Server Code
		private void SetupServer ()
		{
			IPAddress ipAddr = IPAddress.Parse ("127.0.0.1");
			IPEndPoint localEP = new IPEndPoint (ipAddr, 6001);
			
			Socket listener = new Socket (localEP.Address.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
			
			try {
				listener.Bind (localEP);
				listener.Listen (10);
				
				while (true) {
					allDone.Reset ();
					
					Console.WriteLine ("Waiting for a connection...");
					listener.BeginAccept (new AsyncCallback (acceptCallback), listener);
					
					allDone.WaitOne ();
				}
			} catch (Exception e) {
				Console.WriteLine (e.ToString ());
			}
			Console.WriteLine ("Closing the listener...");
		}

		public void acceptCallback (IAsyncResult ar)
		{
			allDone.Set ();
			
			Socket listener = (Socket)ar.AsyncState;
			Socket handler = listener.EndAccept (ar);
			
			StateObject state = new StateObject ();
			state.workSocket = handler;
			handler.BeginReceive (state.buffer, 0, StateObject.BufferSize, 0, new AsyncCallback (ReadCallback), state);
		}

		public void ReadCallback (IAsyncResult ar)
		{
			byte[] byteContent;
			
			// Retrieve the state object and the handler socket
			// from the asynchronous state object.
			StateObject state = (StateObject)ar.AsyncState;
			Socket handler = state.workSocket;
			
			// Read data from the client socket.
			
			int bytesRead = handler.EndReceive (ar);
			
			if (bytesRead > 0) {
				// There  might be more data, so store the data received so far.
				BinaryWriter bWriter = new BinaryWriter (state.mStream);
				bWriter.Write (state.buffer);
				
				byteContent = state.mStream.ToArray ();
				//byteContent = state.mStream.ToArray ();
				
				//Check to see if its as long as the as3kinect buffer is (6).
				// if not, read more data.
				if (byteContent.Length == 6) {
					// All the data has been read
					//Send (handler, byteContent);
					processClientRequest (byteContent, handler);
					Console.WriteLine ("Read in message from client!!! " + byteContent[1].ToString ());
				} else {
					// Not all data received. Get more.
					handler.BeginReceive (state.buffer, 0, StateObject.BufferSize, 0, new AsyncCallback (ReadCallback), state);
				}
			}
		}

		private void Send (Socket handler, byte[] data)
		{
			handler.BeginSend (data, 0, data.Length, 0, new AsyncCallback (SendCallback), handler);
		}

		private void SendCallback (IAsyncResult ar)
		{
			try {
				// Retrieve the socket from the state object.
				Socket handler = (Socket)ar.AsyncState;
				
				// Complete sending the data to the remote device.
				int bytesSent = handler.EndSend (ar);
				Console.WriteLine ("Sent {0} bytes to client.", bytesSent);
				
				handler.Shutdown (SocketShutdown.Both);
				handler.Close ();
				
			} catch (Exception e) {
				Console.WriteLine (e.ToString ());
			}
		}

		// State object for reading client data asynchronously
		public class StateObject
		{
			// Client  socket.
			public Socket workSocket = null;
			// Size of receive buffer.
			public const int BufferSize = 6;
			// Receive buffer.
			public byte[] buffer = new byte[BufferSize];
			// Received data string.
			public MemoryStream mStream = new MemoryStream ();
			//public StringBuilder sb = new StringBuilder ();
		}
		#endregion
		
		
	}
}

