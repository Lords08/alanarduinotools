﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace GbReaper.Forms {
    public partial class FrmNewMap : Form {
        public int CreateWidth { get { return (int)undWidth.Value; } }
        public int CreateHeight { get { return (int)undHeight.Value; } }
        public string CreateName { get { return txbMapname.Text; } }

        public FrmNewMap() {
            InitializeComponent();
        }

        private void btnCreate_Click(object sender, EventArgs e) {
            this.DialogResult = System.Windows.Forms.DialogResult.OK;
        }

        private void btnCancel_Click(object sender, EventArgs e) {
            this.DialogResult = System.Windows.Forms.DialogResult.Cancel;
        }

        private void FrmNewMap_Load(object sender, EventArgs e) {
            txbMapname.Text = string.Format("Map_{0:yyyyMMddHHmmss}", DateTime.Now);
        }
    }
}
