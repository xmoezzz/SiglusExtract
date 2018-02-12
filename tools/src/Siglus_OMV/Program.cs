using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Diagnostics;
using System.Reflection;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Globalization;

namespace OMVUnpacker
{
    class Program
    {
        static void Main(string[] args)
        {
            if (args.Length < 1)
            {
                Console.Error.WriteLine("Usage: omv20120513 infile [infile2 ...]");
                Environment.ExitCode = 1;
                return;
            }
            /*
            string infile;
            string outdir;
            infile = args[0];
            if (args.Length < 2)
            {
                outdir = Path.ChangeExtension(infile, null);
            }
            else
            {
                outdir = args[1];
            }
            */
            foreach (string infile in args)
            {
                // string tmpfile1 = Path.GetTempFileName();
                string tmpfile1 = Path.ChangeExtension(infile, "ogg");
                string outdir = Path.ChangeExtension(infile, null);
                Process p = null;
                try
                {
                    ParseOmv(infile, tmpfile1);
                    p = DecodeTheora(tmpfile1);
                    DecodePictures(p.StandardOutput.BaseStream, outdir);
                }
                catch (Exception e)
                {
                    Console.Error.WriteLine(e.Message);
                }
                finally
                {
                    try
                    {
                        if (!p.HasExited)
                        {
                            p.Kill();
                            p.WaitForExit(5000);
                        }
                        // File.Delete(tmpfile1);
                    }
                    catch (Exception e)
                    {
                        Console.Error.WriteLine(e.Message);
                    }
                }
            }
        }

        static readonly int[] signature = new int[] { 0x4f, 0x67, 0x67, 0x53 }; // "OggS"

        private static void ParseOmv(string infile, string outfile)
        {
            int k = 0;
            using (var fi = new FileStream(infile, FileMode.Open))
            using (var fo = new FileStream(outfile, FileMode.Create))
            {
                // Search for OGG header
                int b;
                while ((b = fi.ReadByte()) != -1)
                {
                    if (b == signature[k])
                    {
                        k++;
                        if (k == signature.Length)
                        {
                            break;
                        }
                    }
                    else
                    {
                        // No embedding, KMP not needed
                        k = 0;
                    }
                }
                fi.Seek(-signature.Length, SeekOrigin.Current);
                fi.CopyTo(fo);
            }
        }

        private static Process DecodeTheora(string infile)
        {
            ProcessStartInfo psi = new ProcessStartInfo(
                Path.Combine(Path.GetDirectoryName(Assembly.GetAssembly(typeof(Program)).Location), "ffmpeg.exe"),
                "-i \"" + infile.Replace("\"", "\"\"") + "\" -f yuv4mpegpipe -"
                );
            psi.UseShellExecute = false;
            psi.RedirectStandardOutput = true;
            // psi.RedirectStandardError = true;
            return Process.Start(psi);
        }

        private static Tuple<string, bool> ReadChunk(Stream f)
        {
            var sb = new StringBuilder();
            bool last = true;
            int b;
            while ((b = f.ReadByte()) != -1)
            {
                if (b == 0x0a || b == 0x0d)
                {
                    break;
                }
                if (b == 0x20 || b == 0x09)
                {
                    last = false;
                    break;
                }
                sb.Append(Convert.ToChar(b));
            }
            return Tuple.Create(sb.ToString(), last);
        }

        private static void DecodePictures(Stream input, string outdir)
        {
            int vidwidth = 0;
            int vidheight = 0;
            int width = 0;
            int height = 0;
            if (!Directory.Exists(outdir))
            {
                Directory.CreateDirectory(outdir);
            }
            using (var logFile = new StreamWriter(Path.Combine(outdir, "log.txt")))
            {
                var c = ReadChunk(input);
                if (c.Item1 != "YUV4MPEG2" || c.Item2)
                {
                    string msg = "Invalid y4m file";
                    logFile.WriteLine(msg);
                    throw new Exception(msg);
                }
                while (!(c = ReadChunk(input)).Item2)
                {
                    string msg = null;
                    var s = c.Item1;
                    switch (s[0])
                    {
                        case 'W':
                            vidwidth = int.Parse(s.Substring(1));
                            width = vidwidth;
                            msg = string.Format("Width: {0}", width);
                            break;
                        case 'H':
                            vidheight = int.Parse(s.Substring(1));
                            height = vidheight * 3 / 4;
                            msg = string.Format("Height: {0}", height);
                            break;
                        case 'F':
                            msg = string.Format("FPS: {0}", s.Substring(1).Replace(":", "/"));
                            break;
                        case 'I':
                            switch (s)
                            {
                                case "Ip":
                                    msg = "Scan type: Progressive";
                                    break;
                                case "It":
                                    msg = "Scan type: Interlaced TFF";
                                    break;
                                case "Ib":
                                    msg = "Scan type: Interlaced BFF";
                                    break;
                                case "Im":
                                    msg = "Scan type: Mixed types";
                                    break;
                                default:
                                    msg = string.Format("Scan type: Unknown ({0})", s.Substring(1));
                                    break;
                            }
                            break;
                        case 'A':
                            msg = string.Format("PAR: {0}", s.Substring(1));
                            break;
                        case 'C':
                            if (s != "C444")
                            {
                                msg = string.Format("Invalid pixel type \"{0}\"", s.Substring(1));
                                logFile.WriteLine(msg);
                                throw new Exception(msg);
                            }
                            msg = "Pixel type: 4:4:4";
                            break;
                        case 'X':
                            msg = string.Format("Comment: {0}", s.Substring(1));
                            break;
                        default:
                            msg = string.Format("Unknown y4m header: {0}", s);
                            logFile.WriteLine(msg);
                            throw new Exception(msg);
                    }
                    logFile.WriteLine(msg);
                    Console.Error.WriteLine(msg);
                }
            }
            var stderrbuf = new byte[4096];
            var vidbuf = new byte[vidwidth * vidheight * 3];
            var bitmap = new Bitmap(width, height, PixelFormat.Format32bppArgb);
            var rect = new Rectangle(0, 0, width, height);
            for (int frame = 0; ; frame++)
            {
                var headerbuf = new byte[6];
                int p = 0;
                int b;
                while ((b = input.Read(headerbuf, p, 6 - p)) > 0)
                {
                    p += b;
                    if (p == 6)
                    {
                        break;
                    }
                }
                if (b == 0)
                {
                    break;
                }
                p = 0;
                while ((b = input.Read(vidbuf, p, vidwidth * vidheight * 3 - p)) > 0)
                {
                    p += b;
                    if (p == vidwidth * vidheight * 3)
                    {
                        break;
                    }
                }
                if (b == 0)
                {
                    break;
                }
                var data = bitmap.LockBits(rect, ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
                var buf = new byte[Math.Abs(data.Stride) * data.Height];
                int offset = data.Stride < 0 ? -data.Stride * (data.Height - 1) : 0;
                for (int y = 0; y < height; y++)
                {
                    for (int x = 0; x < width; x++)
                    {
                        buf[offset + data.Stride * y + 4 * x + 0] = vidbuf[vidwidth * (vidheight * 0 + y) + x];
                        buf[offset + data.Stride * y + 4 * x + 1] = vidbuf[vidwidth * (vidheight * 1 + y) + x];
                        buf[offset + data.Stride * y + 4 * x + 2] = vidbuf[vidwidth * (vidheight * 2 + y) + x];
                        if (y < (height + 2) / 3)
                        {
                            buf[offset + data.Stride * y + 4 * x + 3] = vidbuf[vidwidth * (height * 1 + y) + x];
                        }
                        else if (y < (height + 2) / 3 * 2)
                        {
                            buf[offset + data.Stride * y + 4 * x + 3] = vidbuf[vidwidth * (height * 2 + y) + x];
                        }
                        else
                        {
                            buf[offset + data.Stride * y + 4 * x + 3] = vidbuf[vidwidth * (height * 3 + y) + x];
                        }
                    }
                }
                Marshal.Copy(buf, 0, data.Scan0, buf.Length);
                bitmap.UnlockBits(data);
                bitmap.Save(Path.Combine(outdir, string.Format(CultureInfo.InvariantCulture, "{0:000000}.png", frame)));
            }
        }
    }
}
