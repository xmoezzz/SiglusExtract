using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using System.Drawing.Text;

namespace SiglusExtractCfg
{
    public partial class Form1 : Form
    {
        private SettingParser m_Parser = null;
        public Form1()
        {
            m_Parser = new SettingParser();
            InitializeComponent();

            GameexeTextBox.Text = m_Parser.GetGameexe();
            SceneTextBox.Text   = m_Parser.GetScene();
            if (m_Parser.GetPatchDVD())
                DVDComboBox.SelectedIndex = 0;
            else
                DVDComboBox.SelectedIndex = 1;

            if (m_Parser.GetPatchFont())
                BypassFontComboBox.SelectedIndex = 0;
            else
                BypassFontComboBox.SelectedIndex = 1;

            InstalledFontCollection lf = new InstalledFontCollection();
            int CurrentIndex = 0;
            int SelectedIndex = -1;
            foreach (var font in lf.Families)
            {
                if (font.Name.Length == 0)
                    continue;

                CustomFontComboBox.Items.Add(font.Name);
                if (SelectedIndex == -1 && font.Name == m_Parser.GetCustomFont())
                    SelectedIndex = CurrentIndex;
                CurrentIndex++;
            }

            if (SelectedIndex != -1)
                CustomFontComboBox.SelectedIndex = SelectedIndex;
            else
                CustomFontComboBox.SelectedIndex = 0;
        }

        private void GameexeTextBox_TextChanged(object sender, EventArgs e)
        {
            m_Parser.SetGameexe(GameexeTextBox.Text.Trim());
        }

        private void SceneTextBox_TextChanged(object sender, EventArgs e)
        {
            m_Parser.SetScene(SceneTextBox.Text.Trim());
        }

        private void SaveButton_Click(object sender, EventArgs e)
        {
            //check current setting
            if(!File.Exists(m_Parser.GetGameexe()))
            {
                MessageBox.Show("Couldn't find Gameexe file : " + m_Parser.GetGameexe(), "Gameexe not found", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            if(!File.Exists(m_Parser.GetScene()))
            {
                MessageBox.Show("Couldn't find Scene file : " + m_Parser.GetGameexe(), "Scene script not found", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            if (m_Parser.WriteToFile())
                MessageBox.Show("Save ok!\n!!!Please restart game to apply new setting!!!", "Save ok", MessageBoxButtons.OK);
            else
                MessageBox.Show("Failed to save:(", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }

        private void DVDComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            if(sender == null)
                return;

            if (((ComboBox)sender).SelectedIndex == 0)
                m_Parser.SetPatchDVD(true);
            else
            {
                DialogResult Result = MessageBox.Show("DVD check bypass : NO\nDisable this flag only if game program is already patched(DVD patch)\nClick Yes : Disable\nClick No : Return",
                    "Warning", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);

                if(Result == DialogResult.No)
                {
                    m_Parser.SetPatchDVD(true);
                    ((ComboBox)sender).SelectedIndex = 0;
                }
                else
                {
                    m_Parser.SetPatchDVD(false);
                    ((ComboBox)sender).SelectedIndex = 1;
                }
            }
        }

        private void BypassFontComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (sender == null)
                return;

            if (((ComboBox)sender).SelectedIndex == 0)
                m_Parser.SetPatchDVD(true);
            else
            {
                DialogResult Result = MessageBox.Show("Font limitation bypass : NO\nDisable this flag only if game program is already patched(font patch)\nClick Yes : Disable\nClick No : Return",
                    "Warning", MessageBoxButtons.YesNo, MessageBoxIcon.Warning);

                if (Result == DialogResult.No)
                {
                    m_Parser.SetPatchDVD(true);
                    ((ComboBox)sender).SelectedIndex = 0;
                }
                else
                {
                    m_Parser.SetPatchDVD(false);
                    ((ComboBox)sender).SelectedIndex = 1;
                }
            }
        }

        private void CustomFontComboBox_SelectedIndexChanged(object sender, EventArgs e)
        {
            int Index = ((ComboBox)sender).SelectedIndex;
            m_Parser.SetCustomFont((string)((ComboBox)sender).Items[Index]);
        }
    }
}
