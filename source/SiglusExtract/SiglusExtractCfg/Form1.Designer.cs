namespace SiglusExtractCfg
{
    partial class Form1
    {
        /// <summary>
        /// 必需的设计器变量。
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 清理所有正在使用的资源。
        /// </summary>
        /// <param name="disposing">如果应释放托管资源，为 true；否则为 false。</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows 窗体设计器生成的代码

        /// <summary>
        /// 设计器支持所需的方法 - 不要
        /// 使用代码编辑器修改此方法的内容。
        /// </summary>
        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.GameexeTextBox = new System.Windows.Forms.TextBox();
            this.GameexeLabel = new System.Windows.Forms.Label();
            this.SceneLabel = new System.Windows.Forms.Label();
            this.SceneTextBox = new System.Windows.Forms.TextBox();
            this.DVDComboBox = new System.Windows.Forms.ComboBox();
            this.DVDLabel = new System.Windows.Forms.Label();
            this.BypassFontLabel = new System.Windows.Forms.Label();
            this.BypassFontComboBox = new System.Windows.Forms.ComboBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.label1 = new System.Windows.Forms.Label();
            this.CustomFontComboBox = new System.Windows.Forms.ComboBox();
            this.SaveButton = new System.Windows.Forms.Button();
            this.NotificationLabel = new System.Windows.Forms.Label();
            this.CfgHelpProvider = new System.Windows.Forms.HelpProvider();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // GameexeTextBox
            // 
            this.CfgHelpProvider.SetHelpString(this.GameexeTextBox, "Custom game config file name (eg : Gameexe_en.dat)");
            this.GameexeTextBox.Location = new System.Drawing.Point(61, 23);
            this.GameexeTextBox.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.GameexeTextBox.Name = "GameexeTextBox";
            this.CfgHelpProvider.SetShowHelp(this.GameexeTextBox, true);
            this.GameexeTextBox.Size = new System.Drawing.Size(140, 20);
            this.GameexeTextBox.TabIndex = 0;
            this.GameexeTextBox.TextChanged += new System.EventHandler(this.GameexeTextBox_TextChanged);
            // 
            // GameexeLabel
            // 
            this.GameexeLabel.AutoSize = true;
            this.GameexeLabel.Location = new System.Drawing.Point(5, 25);
            this.GameexeLabel.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.GameexeLabel.Name = "GameexeLabel";
            this.GameexeLabel.Size = new System.Drawing.Size(52, 13);
            this.GameexeLabel.TabIndex = 1;
            this.GameexeLabel.Text = "Gameexe";
            // 
            // SceneLabel
            // 
            this.SceneLabel.AutoSize = true;
            this.SceneLabel.Location = new System.Drawing.Point(215, 31);
            this.SceneLabel.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.SceneLabel.Name = "SceneLabel";
            this.SceneLabel.Size = new System.Drawing.Size(38, 13);
            this.SceneLabel.TabIndex = 3;
            this.SceneLabel.Text = "Scene";
            // 
            // SceneTextBox
            // 
            this.CfgHelpProvider.SetHelpString(this.SceneTextBox, "Custom scene script name (eg : Scene_en.pck)");
            this.SceneTextBox.Location = new System.Drawing.Point(267, 28);
            this.SceneTextBox.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.SceneTextBox.Name = "SceneTextBox";
            this.CfgHelpProvider.SetShowHelp(this.SceneTextBox, true);
            this.SceneTextBox.Size = new System.Drawing.Size(140, 20);
            this.SceneTextBox.TabIndex = 2;
            this.SceneTextBox.TextChanged += new System.EventHandler(this.SceneTextBox_TextChanged);
            // 
            // DVDComboBox
            // 
            this.DVDComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.DVDComboBox.FormattingEnabled = true;
            this.CfgHelpProvider.SetHelpString(this.DVDComboBox, "Yes : bypass DVD check(recommend)\\\\n No: Do nothing for this check Choose \'No\' ON" +
        "LY if game program is already patched");
            this.DVDComboBox.Items.AddRange(new object[] {
            "Yes",
            "No"});
            this.DVDComboBox.Location = new System.Drawing.Point(106, 65);
            this.DVDComboBox.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.DVDComboBox.Name = "DVDComboBox";
            this.CfgHelpProvider.SetShowHelp(this.DVDComboBox, true);
            this.DVDComboBox.Size = new System.Drawing.Size(95, 21);
            this.DVDComboBox.TabIndex = 4;
            this.DVDComboBox.SelectedIndexChanged += new System.EventHandler(this.DVDComboBox_SelectedIndexChanged);
            // 
            // DVDLabel
            // 
            this.DVDLabel.AutoSize = true;
            this.DVDLabel.Location = new System.Drawing.Point(5, 67);
            this.DVDLabel.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.DVDLabel.Name = "DVDLabel";
            this.DVDLabel.Size = new System.Drawing.Size(100, 13);
            this.DVDLabel.TabIndex = 5;
            this.DVDLabel.Text = "Bypass DVD check";
            // 
            // BypassFontLabel
            // 
            this.BypassFontLabel.AutoSize = true;
            this.BypassFontLabel.Location = new System.Drawing.Point(204, 67);
            this.BypassFontLabel.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.BypassFontLabel.Name = "BypassFontLabel";
            this.BypassFontLabel.Size = new System.Drawing.Size(95, 13);
            this.BypassFontLabel.TabIndex = 7;
            this.BypassFontLabel.Text = "Bypass font check";
            // 
            // BypassFontComboBox
            // 
            this.BypassFontComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.BypassFontComboBox.FormattingEnabled = true;
            this.CfgHelpProvider.SetHelpString(this.BypassFontComboBox, "Yes : bypass font limitation(recommend)\\\\n No: Do nothing for this check.  Choose" +
        " \'No\' ONLY if game program is already patched");
            this.BypassFontComboBox.Items.AddRange(new object[] {
            "Yes",
            "No"});
            this.BypassFontComboBox.Location = new System.Drawing.Point(312, 71);
            this.BypassFontComboBox.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.BypassFontComboBox.Name = "BypassFontComboBox";
            this.CfgHelpProvider.SetShowHelp(this.BypassFontComboBox, true);
            this.BypassFontComboBox.Size = new System.Drawing.Size(95, 21);
            this.BypassFontComboBox.TabIndex = 6;
            this.BypassFontComboBox.SelectedIndexChanged += new System.EventHandler(this.BypassFontComboBox_SelectedIndexChanged);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Controls.Add(this.CustomFontComboBox);
            this.groupBox1.Controls.Add(this.BypassFontLabel);
            this.groupBox1.Controls.Add(this.GameexeTextBox);
            this.groupBox1.Controls.Add(this.GameexeLabel);
            this.groupBox1.Controls.Add(this.DVDLabel);
            this.groupBox1.Controls.Add(this.DVDComboBox);
            this.groupBox1.Location = new System.Drawing.Point(11, 6);
            this.groupBox1.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Padding = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.groupBox1.Size = new System.Drawing.Size(403, 129);
            this.groupBox1.TabIndex = 8;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Setting";
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(39, 100);
            this.label1.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(63, 13);
            this.label1.TabIndex = 9;
            this.label1.Text = "Custom font";
            // 
            // CustomFontComboBox
            // 
            this.CustomFontComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.CustomFontComboBox.FormattingEnabled = true;
            this.CfgHelpProvider.SetHelpString(this.CustomFontComboBox, "Choose custom font");
            this.CustomFontComboBox.Location = new System.Drawing.Point(106, 98);
            this.CustomFontComboBox.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.CustomFontComboBox.Name = "CustomFontComboBox";
            this.CfgHelpProvider.SetShowHelp(this.CustomFontComboBox, true);
            this.CustomFontComboBox.Size = new System.Drawing.Size(290, 21);
            this.CustomFontComboBox.TabIndex = 8;
            this.CustomFontComboBox.SelectedIndexChanged += new System.EventHandler(this.CustomFontComboBox_SelectedIndexChanged);
            // 
            // SaveButton
            // 
            this.SaveButton.Location = new System.Drawing.Point(358, 156);
            this.SaveButton.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.SaveButton.Name = "SaveButton";
            this.SaveButton.Size = new System.Drawing.Size(56, 19);
            this.SaveButton.TabIndex = 9;
            this.SaveButton.Text = "Save";
            this.SaveButton.UseVisualStyleBackColor = true;
            this.SaveButton.Click += new System.EventHandler(this.SaveButton_Click);
            // 
            // NotificationLabel
            // 
            this.NotificationLabel.AutoSize = true;
            this.NotificationLabel.ForeColor = System.Drawing.Color.Red;
            this.NotificationLabel.Location = new System.Drawing.Point(9, 137);
            this.NotificationLabel.Margin = new System.Windows.Forms.Padding(2, 0, 2, 0);
            this.NotificationLabel.Name = "NotificationLabel";
            this.NotificationLabel.Size = new System.Drawing.Size(380, 13);
            this.NotificationLabel.TabIndex = 10;
            this.NotificationLabel.Text = "* You MUST restart game to apply setting \\ Use SiglusExtract\'s In-Game setting";
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(425, 184);
            this.Controls.Add(this.NotificationLabel);
            this.Controls.Add(this.SaveButton);
            this.Controls.Add(this.BypassFontComboBox);
            this.Controls.Add(this.SceneLabel);
            this.Controls.Add(this.SceneTextBox);
            this.Controls.Add(this.groupBox1);
            this.DoubleBuffered = true;
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.HelpButton = true;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.Margin = new System.Windows.Forms.Padding(2, 2, 2, 2);
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "Form1";
            this.Opacity = 0.9D;
            this.Text = "SiglusExtract config";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TextBox GameexeTextBox;
        private System.Windows.Forms.Label GameexeLabel;
        private System.Windows.Forms.Label SceneLabel;
        private System.Windows.Forms.TextBox SceneTextBox;
        private System.Windows.Forms.ComboBox DVDComboBox;
        private System.Windows.Forms.Label DVDLabel;
        private System.Windows.Forms.Label BypassFontLabel;
        private System.Windows.Forms.ComboBox BypassFontComboBox;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.ComboBox CustomFontComboBox;
        private System.Windows.Forms.Button SaveButton;
        private System.Windows.Forms.Label NotificationLabel;
        private System.Windows.Forms.HelpProvider CfgHelpProvider;
    }
}

