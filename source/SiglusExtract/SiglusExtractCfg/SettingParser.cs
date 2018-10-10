using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Forms;
using System.Management;
using System.Security.Cryptography;
using System.Runtime.InteropServices;

namespace SiglusExtractCfg
{
    class SettingParser
    {
        public string m_Gameexe   = "Gameexe.dat";
        public string m_Scene     = "Scene.pck";
        public bool   m_PatchDVD  = true;
        public bool   m_PatchFont = true;
        public string m_UserFont  = "SimHei";


        private string GetDefGameexe()
        {
            return "Gameexe.dat";
        }

        private string GetDefScene()
        {
            return "Scene.pck";
        }

        private string GetDefPatchDVD()
        {
            return "true";
        }

        private string GetDefPatchFont()
        {
            return "true";
        }

        private string GetDefUserFont()
        {
            return "SimHei";
        }

        public SettingParser()
        {
            bool Success = false;

            Success = LoadFromFile();
            if (!Success)
                ResetDefault();

            WriteToFile();
        }

        [DllImport("kernel32.dll",CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        private static extern bool WritePrivateProfileStringW(string AppName, string Key, string Val, string FilePath);

        [DllImport("kernel32.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        private static extern int  GetPrivateProfileStringW(string AppName, string KeyName, string Default, byte[] ReturnedString, int nSize, string FilePath);

        public void   SetGameexe(string Gameexe) { m_Gameexe = Gameexe; }
        public string GetGameexe() { return m_Gameexe; }
        public void   SetScene(string Scene) { m_Scene = Scene; }
        public string GetScene() { return m_Scene; }
        public void   SetPatchDVD(bool val) { m_PatchDVD = val; }
        public bool   GetPatchDVD() { return m_PatchDVD; }
        public void   SetPatchFont(bool val) { m_PatchFont = val; }
        public bool   GetPatchFont() { return m_PatchFont; }
        public void   SetCustomFont(string font) { m_UserFont = font; }
        public string GetCustomFont() { return m_UserFont; }

        private static string AppNameCfg = "SiglusCfg";
        private static string FilePathCfg = "./SiglusCfg.ini";

        private string BoolToString(bool val)
        {
            if (val)
                return "true";

            return "false";
        }

        public bool WriteToFile()
        {
            bool Success = false;

            do
            {
                Success = WritePrivateProfileStringW(AppNameCfg, "Gameexe", GetGameexe(), FilePathCfg);
                if (!Success)
                    break;

                Success = WritePrivateProfileStringW(AppNameCfg, "Scene", GetScene(), FilePathCfg);
                if (!Success)
                    break;

                Success = WritePrivateProfileStringW(AppNameCfg, "BypassDVD", BoolToString(GetPatchDVD()), FilePathCfg);
                if (!Success)
                    break;

                Success = WritePrivateProfileStringW(AppNameCfg, "BypassFont", BoolToString(GetPatchFont()), FilePathCfg);
                if (!Success)
                    break;

                Success = WritePrivateProfileStringW(AppNameCfg, "CustomFont", GetCustomFont(), FilePathCfg);
                if (!Success)
                    break;

            } while (false);

            return Success;
        }


        public string ReadIni(string Key, string Default)
        {　　　　　　
            Byte[] Buffer = new Byte[1000];　　　　　　
            int Len = GetPrivateProfileStringW(AppNameCfg, Key, Default, Buffer, Buffer.GetUpperBound(0), FilePathCfg);　
            string s = Encoding.Unicode.GetString(Buffer);　　　　　　
            s = s.Substring(0, Len);　　　　　　
            return s.Trim();　　　　
        }

        private bool LoadFromFile()
        {
            string RetString;

            if (!File.Exists(FilePathCfg))
                return false;

            RetString = ReadIni("Gameexe", GetDefGameexe());
            if (RetString.Length != 0)
                SetGameexe(RetString);

            RetString = ReadIni("Scene", GetDefScene());
            if (RetString.Length != 0)
                SetScene(RetString);

            RetString = ReadIni("BypassDVD", GetDefPatchDVD());
            if (RetString.Length != 0)
                SetPatchDVD(RetString == "true" ? true : false);

            RetString = ReadIni("BypassFont", GetDefPatchFont());
            if (RetString.Length != 0)
                SetPatchFont(RetString == "true" ? true : false);

            RetString = ReadIni("CustomFont", GetDefUserFont());
            if (RetString.Length != 0)
                SetCustomFont(RetString);

            return true;
        }

        public bool ResetDefault()
        {
            m_Gameexe   = "Gameexe.dat";
            m_Scene     = "Scene.pck";
            m_PatchDVD  = true;
            m_PatchFont = true;
            m_UserFont  = "SimHei";
            return true;
        }
    }
}
