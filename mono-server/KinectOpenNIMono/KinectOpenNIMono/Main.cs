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

		private xn.Context context;
		private Thread readerThread;
		private bool shouldRun;
		private xnv.SessionManager sessionManager;
		private xnv.PointControl pointControl;
		private xnv.HandPointContext primaryHand;
		
		private List<xnv.HandPointContext> handPoints;
		
		private TcpListener tcpListener;
		private Thread listenThread;
		
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

		
		List<byte[]> processClientRequest (byte[] data)
		{
			//byte cameraID = data[0];
			byte requestNum = data[1];
			List<byte[]> responseBuffers = new List<byte[]> ();
			
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
					
					responseBuffers.Add (ms.ToArray ());
				}
				
				if (responseBuffers.ToArray ().Length == 0) {
					
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
					
					responseBuffers.Add (ba);
				}
				break;
			default:
				break;
			}
			return responseBuffers;
		}

		#region Async Server Code
		void SetupServer ()
		{
			this.tcpListener = new TcpListener (IPAddress.Parse ("127.0.0.1"), 6001);
			this.listenThread = new Thread (new ThreadStart (ListenForClients));
			this.listenThread.Start ();
		}
		
		private void ListenForClients ()
		{
			this.tcpListener.Start ();
			Console.WriteLine ("Waiting for client to connect...");
			while (true) {
				//blocks until a client has connected to the server
				
				TcpClient client = this.tcpListener.AcceptTcpClient ();
				Console.WriteLine ("Client has connected!");
				//create a thread to handle communication 
				//with connected client
				Thread clientThread = new Thread (new ParameterizedThreadStart (HandleClientComm));
				clientThread.Start (client);
			}
		}
		
		private void HandleClientComm (object client)
		{
			TcpClient tcpClient = (TcpClient)client;
			NetworkStream clientStream = tcpClient.GetStream ();
			
			byte[] message = new byte[6];
			int bytesRead;
			
			while (true) {
				bytesRead = 0;
				
				try {
					//blocks until a client sends a message
					bytesRead = clientStream.Read (message, 0, 6);
					Console.WriteLine ("Message recieved from client!");
				} catch {
					Console.WriteLine ("Socket Error!");
					break;
				}
				
				if (bytesRead == 0) {
					//the client has disconnected from the server
					Console.WriteLine ("Client has disconnected.");
					break;
				}
				
				List<byte[]> buffersToSend = processClientRequest (message);
				foreach (byte[] response in buffersToSend) {
					clientStream.Write (response, 0, response.Length);
					clientStream.Flush ();
					Console.WriteLine ("Sent message to client!");
				}
				
			}
			
			tcpClient.Close ();
		}
		
		#endregion
		
		
	}
}

