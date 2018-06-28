using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Extensibility;
using Microsoft.Office.Interop.Outlook;

namespace Falcon.Desktop
{
    public class Gosh: IDTExtensibility2
    {
        private Application application;

        public void OnConnection(object Application, ext_ConnectMode ConnectMode, object AddInInst, ref Array custom)
        {
            application = Application as Application;
            EscDialog dialog = new EscDialog();
            dialog.ShowDialog();
        }

        public void OnDisconnection(ext_DisconnectMode RemoveMode, ref Array custom)
        {
            throw new NotImplementedException();
        }

        public void OnAddInsUpdate(ref Array custom)
        {
            throw new NotImplementedException();
        }

        public void OnStartupComplete(ref Array custom)
        {
            throw new NotImplementedException();
        }

        public void OnBeginShutdown(ref Array custom)
        {
            throw new NotImplementedException();
        }
    }
}
