using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Falcon.Desktop
{
    public partial class EscDialog : Form
    {
        public EscDialog()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            EscDialog dialog = new EscDialog();
            dialog.ShowDialog();
        }

        protected override void OnActivated(EventArgs e)
        {
            base.OnActivated(e);
            Console.WriteLine("OnActivated");
        }

        protected override void OnDeactivate(EventArgs e)
        {
            base.OnDeactivate(e);
            Console.WriteLine("OnDeActivated");
            System.Threading.Thread.Sleep(10000);
        }
    }
}
