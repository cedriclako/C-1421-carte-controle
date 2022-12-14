using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using CommandLine;
using System.Linq;
using System.Text;
using System.IO.Compression;

namespace res2code
{
    class Program
    {
        public class Options
        {
            [Option('i', "inputpath", Required = false, HelpText = "Input path (files to embed)")]
            public string InputPath { get; set; } = "";

            [Option('o', "outputcodepath", Required = false, HelpText = "Output path for code")]
            public string OutputCodePath { get; set; } = "";

            [Option('c', "compresscfgfile", Required = false, HelpText = "Input compression configuration file")]
            public string CompressConfigFilename { get; set; } = "";

            [Option("outbin", Required = false, HelpText = "Make a .bin file")]
            public bool IsOutBin { get; set; } = false;

            [Option("outlisting", Required = false, HelpText = "Create listing .txt")]
            public bool IsOutListing { get; set; } = false;

            [Option("assetname", Required = false, HelpText = "Name for assets generation")]
            public string? AssetName { get; set; } = null;
        }

        public class ScanDir
        {
            public readonly DirectoryInfo Root;
            public readonly string RelativePath;

            public ScanDir(DirectoryInfo root, string relativePath)
            {
                this.Root = root;
                this.RelativePath = relativePath;
            }
        }

        public class ScanFI
        {
            public readonly ScanDir RootScanDir;
            public readonly FileInfo FileInfo;
            public readonly string RelativeFilename;

            public ScanFI(ScanDir rootScanDir, FileInfo fileInfo, string relativeFilename)
            {
                this.RootScanDir = rootScanDir;
                this.FileInfo = fileInfo;
                this.RelativeFilename = relativeFilename;
            }

            public long BlobAddress { get; set; } = 0;

            public byte[] Datas { get; set; } = new byte[0];

            public bool IsGZipCompressed { get; set; } = false;

            public override string ToString() => RelativeFilename;
        }

        static int Main(string[] args)
        {
            try
            {
                int error = 0;

                Parser.Default.ParseArguments<Options>(args)
                       .WithParsed<Options>(o =>
                       {
                           DirectoryInfo diInput = new DirectoryInfo(o.InputPath);
                           if (!diInput.Exists)
                               throw new Exception("Input path doesn't exists");

                           DirectoryInfo diOutputCodePath = new DirectoryInfo(o.OutputCodePath);
                           diOutputCodePath.Create();
                           if (!diOutputCodePath.Exists)
                               throw new Exception("Output path doesn't exists");

                           bool hasCompressConfig = false;
                           if (!String.IsNullOrEmpty(o.CompressConfigFilename))
                           {
                               if (!File.Exists(o.CompressConfigFilename))
                                   throw new Exception($"No GZIP config file provided");
                               hasCompressConfig = true;
                           }

                           string assetName = o.AssetName ?? "EmbeddedFiles";

                           // Input files
                           Stack<ScanDir> diScans = new Stack<ScanDir>();
                           diScans.Push(new ScanDir(diInput, ""));

                           List<ScanFI> scanFIs = new List<ScanFI>();

                           // List all files
                           while(diScans.Count > 0)
                           {
                               ScanDir currScanDir = diScans.Pop();

                               foreach (DirectoryInfo di in currScanDir.Root.GetDirectories().Reverse())
                                   diScans.Push(new ScanDir(di, $"{ currScanDir.RelativePath }{ di.Name }/"));

                               foreach (FileInfo fiInput in currScanDir.Root.GetFiles())
                                   scanFIs.Add(new ScanFI(currScanDir, fiInput, currScanDir.RelativePath + fiInput.Name));
                           }

                           //// Order by file type
                           scanFIs = scanFIs
                            .OrderBy(p => p.FileInfo, new FileOrdering())
                            .ToList();

                           foreach (ScanFI scanFI in scanFIs)
                               Console.Error.WriteLine($"Scan file: { scanFI.FileInfo.FullName }");

                           // Check if there is a config file.
                           if (hasCompressConfig)
                           {
                               ConfigReader.FileConfig[] fileConfigs = ConfigReader.GetConfig(o.CompressConfigFilename)
                                    .ToArray();

                               // Set scanned file to gzip mode
                               foreach (ScanFI scanFI in scanFIs)
                                   scanFI.IsGZipCompressed = fileConfigs.Any(p => p.RelativeFilename.Equals(scanFI.RelativeFilename, StringComparison.InvariantCultureIgnoreCase));
                           }

                           string NAMETOENUM(string relativeFilename) => $"EF_EFILE_{ FORMATFILENAME(relativeFilename) }";

                           if (o.IsOutListing)
                           {
                               string fileTXT = System.IO.Path.Combine(o.OutputCodePath, Path.ChangeExtension(assetName, ".txt"));
                               Console.Error.WriteLine($"Export list: {fileTXT}");
                               using (StreamWriter sw = new StreamWriter(fileTXT, false, System.Text.Encoding.UTF8))
                               {
                                   foreach (ScanFI scanFI in scanFIs)
                                       sw.WriteLine($"{scanFI.RelativeFilename}");
                               }
                           }


                           byte[] bigBlobs = new byte[0];

                           // Generate .c and .h files
                           string fileHRelativeFilename = Path.ChangeExtension(assetName, ".h");
                           string fileFullNameH = System.IO.Path.Combine(o.OutputCodePath, fileHRelativeFilename);
                           Console.Error.WriteLine($"Generating file: { fileFullNameH }");
                           using(StreamWriter sw = new StreamWriter(fileFullNameH, false, System.Text.Encoding.UTF8))
                           {
                               sw.WriteLine($"#ifndef _{assetName.ToUpper()}_");
                               sw.WriteLine($"#define _{assetName.ToUpper()}_");
                               sw.WriteLine();
                               sw.WriteLine("#include <stdint.h>");
                               sw.WriteLine();
                               sw.WriteLine("typedef enum");
                               sw.WriteLine("{");
                               sw.WriteLine("   EF_EFLAGS_None = 0,");
                               sw.WriteLine("   EF_EFLAGS_GZip = 1,");
                               sw.WriteLine("} EF_EFLAGS;");
                               sw.WriteLine();
                               sw.WriteLine("typedef struct");
                               sw.WriteLine("{");
                               sw.WriteLine("   const char* strFilename;");
                               sw.WriteLine("   uint32_t u32Length;");
                               sw.WriteLine("   EF_EFLAGS eFlags;");
                               sw.WriteLine("   const uint8_t* pu8StartAddr;");
                               sw.WriteLine("} EF_SFile;");
                               sw.WriteLine();
                               sw.WriteLine("typedef enum");
                               sw.WriteLine("{");
                               for (int i = 0; i < scanFIs.Count; i++)
                               {
                                   ScanFI scanFI = scanFIs[i];
                                   sw.Write($"  {NAMETOENUM(scanFI.RelativeFilename)} = {i},    /*!< @brief File: { scanFI.RelativeFilename } */");
                                   //if (i + 1 < scanFIs.Count)
                                   //    sw.Write(",");
                                   sw.WriteLine();
                               }
                               sw.WriteLine($"  {NAMETOENUM("COUNT")} = {scanFIs.Count}");
                               sw.WriteLine("} EF_EFILE;");
                               sw.WriteLine();
                               sw.WriteLine("/*! @brief Check if compressed flag is active */");
                               sw.WriteLine("#define EF_ISFILECOMPRESSED(x) ((x & EF_EFLAGS_GZip) == EF_EFLAGS_GZip)");
                               sw.WriteLine();
                               sw.WriteLine("extern const EF_SFile EF_g_sFiles[EF_EFILE_COUNT];");
                               sw.WriteLine("extern const uint8_t EF_g_u8Blobs[];");
                               sw.WriteLine();
                               sw.WriteLine("#endif");
                           }

                           string fileC = System.IO.Path.Combine(o.OutputCodePath, Path.ChangeExtension(assetName, ".c"));
                           Console.Error.WriteLine($"Generating file: { fileC }");
                           using (StreamWriter sw = new StreamWriter(fileC, false, System.Text.Encoding.UTF8))
                           {
                               sw.WriteLine($"#include \"{fileHRelativeFilename}\"");
                               sw.WriteLine();
                               // Generate variables
                               // We also add one space to add trailing 0. Usefull for string files.
                               // Check all files. not very efficient but .... check it's a bird!
                               for (int i = 0; i < scanFIs.Count; i++)
                               {
                                   ScanFI scanFI = scanFIs[i];
                                   byte[] datas = File.ReadAllBytes(scanFI.FileInfo.FullName);

                                   // Check if GZip is active
                                   if (scanFI.IsGZipCompressed)
                                   {
                                       MemoryStream originalData = new MemoryStream(datas);
                                       MemoryStream msCompressed = new MemoryStream();
                                       using (GZipStream gZipStream = new GZipStream(msCompressed, CompressionLevel.Optimal))
                                       {
                                           originalData.CopyTo(gZipStream);
                                       }
                                       scanFI.Datas = msCompressed.ToArray();
                                   }
                                   else scanFI.Datas = datas;
                               }

                               bigBlobs = new byte[scanFIs.Sum(fi => fi.Datas.Length) + scanFIs.Count];

                               long startAddr = 0;
                               for (int i = 0; i < scanFIs.Count; i++)
                               {
                                   ScanFI scanFI = scanFIs[i];

                                   Array.Copy(scanFI.Datas, 0, bigBlobs, startAddr, scanFI.Datas.Length);
                                   bigBlobs[startAddr + scanFI.Datas.Length] = 0;

                                   scanFI.BlobAddress = startAddr;
                                   startAddr += scanFI.Datas.Length + 1; // For trailing 0.
                               }

                               if (o.IsOutBin)
                               {
                                   // Save the blob
                                   string fileBin = System.IO.Path.Combine(o.OutputCodePath, Path.ChangeExtension(assetName, ".bin"));
                                   using (FileStream swBinBlob = new FileStream(fileBin, FileMode.OpenOrCreate, FileAccess.Write))
                                       swBinBlob.Write(bigBlobs, 0, bigBlobs.Length);
                               }

                               sw.WriteLine();
                               sw.WriteLine($"/*! @brief Total size: { scanFIs.Sum(p => p.FileInfo.Length) }, total (including trailing 0s): { bigBlobs.Length } */");
                               sw.WriteLine("const EF_SFile EF_g_sFiles[EF_EFILE_COUNT] = ");
                               sw.WriteLine("{");
                               for (int i = 0; i < scanFIs.Count; i++)
                               {
                                   ScanFI scanFI = scanFIs[i];
                                   sw.Write($"   [{NAMETOENUM(scanFI.RelativeFilename)}] = {{ \"{ scanFI.RelativeFilename }\", { scanFI.FileInfo.Length }, { (scanFI.IsGZipCompressed ? "EF_EFLAGS_GZip" : "EF_EFLAGS_None") }, &EF_g_u8Blobs[{ scanFI.BlobAddress }] }}");
                                   if (i + 1 < scanFIs.Count)
                                        sw.Write(",");
                                   sw.Write($"/* size: { scanFI.Datas.Length }{ (scanFI.IsGZipCompressed ? $", size (original): { scanFI.FileInfo.Length }" : "") } */");
                                   sw.WriteLine();
                               }
                               sw.WriteLine("};");
                           }

                           string fileBinC = System.IO.Path.Combine(o.OutputCodePath, Path.ChangeExtension(assetName+"Bin", ".c"));
                           Console.Error.WriteLine($"Generating file: {fileBinC}");
                           using (StreamWriter sw = new StreamWriter(fileBinC, false, System.Text.Encoding.UTF8))
                           {
                               sw.WriteLine($"#include \"{fileHRelativeFilename}\"");
                               sw.WriteLine();
                               sw.WriteLine($"const uint8_t EF_g_u8Blobs[] = {{\r\n{BinToHexaArray(bigBlobs)}\r\n}};");
                               sw.WriteLine();
                           }

                       });

                return 0;
            }
            catch (Exception ex)
            {
                Console.Error.WriteLine($"Error: { ex.Message }");
                return -1;
            }
        }

        /// <summary>
        /// Convert to variable name
        /// </summary>
        /// <param name="filename"></param>
        /// <returns></returns>
        static string FORMATFILENAME(string filename) => filename.Replace(" ", "_").Replace("-", "_").Replace("/", "_").Replace(".", "_").ToUpper();

        static string BinToHexaArray(byte[] datas)
        {
            StringBuilder sb = new StringBuilder();
            for (int j = 0; j < datas.Length; j++)
            {
                sb.Append(String.Format("0x{0:X2}", datas[j]));
                if (j + 1 < datas.Length)
                    sb.Append(",");
                if (j % 25 == 0 && j > 0)
                    sb.AppendLine();
            }
            return sb.ToString();
        }
    }
}
