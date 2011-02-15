using System;
using System.Threading;
using System.Net.Sockets;
using System.Net;
using System.IO;
using System.Text;
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
			
			this.pointControl.PointUpdate += new xnv.PointControl.PointUpdateHandler (pointControl_PointUpdate);
			
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

		void pointControl_PointUpdate (ref xnv.HandPointContext context)
		{
			if (!context.Equals (null)) {
				
				string sendData = context.ptPosition.X.ToString () + "|" + context.ptPosition.Y.ToString () + "|" + context.ptPosition.Z.ToString ();
				
				
				Console.Out.WriteLine ("PointUpdate: " + sendData);
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

		public static void ReadCallback (IAsyncResult ar)
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
					Console.WriteLine ("Read in message from client!!! " + byteContent[2].ToString ());
				} else {
					// Not all data received. Get more.
					handler.BeginReceive (state.buffer, 0, StateObject.BufferSize, 0, new AsyncCallback (ReadCallback), state);
				}
			}
		}

		private static void Send (Socket handler, byte[] data)
		{
			handler.BeginSend (data, 0, data.Length, 0, new AsyncCallback (SendCallback), handler);
		}

		private static void SendCallback (IAsyncResult ar)
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
			public MemoryStream mStream = new MemoryStream();
			//public StringBuilder sb = new StringBuilder ();
		}
		#endregion
		
		
	}
}

