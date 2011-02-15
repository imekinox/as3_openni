using System;
using System.Threading;

namespace KinectOpenNIMono
{
	class MainClass
	{
		public static void Main (string[] args)
		{
			MainClass main = new MainClass ();
			main.SetupKinect ();
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
		
		private readonly string SAMPLE_XML_FILE = @"../../Sample-Tracking.xml";

		private xn.Context context;
		private Thread readerThread;
		private bool shouldRun;
		private xnv.SessionManager sessionManager;
		private xnv.PointControl pointControl;
	}
}

