using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Drawing;

namespace CombinePNG
{
    class Program
    {
        public static string PictureFilter = ".PNG;";

        public static List<string> GetFileNames(string pathname)
        {
            List<string> fileList = new List<string>();
            string[] subFiles = Directory.GetFiles(pathname, "*.png");
            foreach (string subFile in subFiles)
            {
                fileList.Add(subFile);
            }

            return fileList;
        }


        public static Image CombinImage(string[] sourceImg)
        {
            Image imgBack = System.Drawing.Image.FromFile(sourceImg[0]);
 
            Graphics g = Graphics.FromImage(imgBack);
            g.DrawImage(imgBack, 0, 0, imgBack.Width, imgBack.Height);

            for (int i = 1; i < sourceImg.Count(); i++ )
            {
                Image img = System.Drawing.Image.FromFile(sourceImg[i]);
                g.DrawImage(img, 0, 0, img.Width, img.Height);
            }
            GC.Collect();
            return imgBack;
        }

        static void Main(string[] args)
        {
            List<string> FileList = GetFileNames(Directory.GetCurrentDirectory());
            if (FileList.Count == 0)
                return;

            string Name = Directory.GetCurrentDirectory();
            Name = Name.Substring(Name.LastIndexOf("\\") + 1);
            Name += ".png";

            System.Console.WriteLine(Name);

            Image Result = CombinImage(FileList.ToArray());
            Result.Save(Name, System.Drawing.Imaging.ImageFormat.Png);
        }
    }
}
