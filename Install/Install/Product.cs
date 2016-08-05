using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Microsoft.Win32;
using System.IO;

namespace Install
{
    class Product
    {
        /// <summary>
        /// 中望CAD
        /// </summary>
        public string Desc { get; set; }

        /// <summary>
        /// zwcad  zwcada
        /// </summary>
        public string Name { get; set; }

        public bool IsX64 { get; set; }

        /// <summary>
        /// 语言，zh-CN
        /// </summary>
        public string Language { get; set; }

        public char Key { get; set; }

        public string FileName { get; set; }

        private bool? m_bExist = null;
        public bool Exist
        {
            get
            {
                if (null == m_bExist)
                {
                    m_bExist = IsExist();
                }

                return m_bExist.Value;
            }
        }

        public string FilePath
        {
            get
            {
                var sPath = Path.GetFullPath(FileName);
                if (!File.Exists(sPath))
                {
                    sPath = Path.Combine(Environment.CurrentDirectory, FileName);
                    sPath = Path.GetFullPath(sPath);
                }

                return sPath;
            }
        }

        public bool DoInstall()
        {
            try
            {
                Console.WriteLine("{0} 正在安装.", Desc);
                string sRegPath = string.Format("Software\\ZWSoft\\{0}\\2017\\{1}\\Applications\\PdfConverter", Name, Language);
                var regApp = Registry.CurrentUser.CreateSubKey(sRegPath);
                regApp.SetValue("LOADER", FilePath);
                regApp.SetValue("LOADCTRLS", 4, RegistryValueKind.DWord);

                var regGroups = regApp.CreateSubKey("Groups");
                regGroups.SetValue("ZOSP_PDF_TOOLs", "ZOSP_PDF_TOOLs");

                var regCmds = regApp.CreateSubKey("Commands");
                regCmds.SetValue("PDF2DWG", "PDF2DWG");
                regCmds.SetValue("PDF2DXF", "PDF2DXF");

                Console.WriteLine("{0} 安装成功", Desc);
                return true;
            }
            catch (System.Exception ex)
            {
                Console.WriteLine("安装 {0} 失败", Desc);
                Console.WriteLine(ex.ToString());
                return false;
            }
        }

        private bool IsExist()
        {
            try
            {
                // 32位系统上不能运行64位程序
                if (IsX64 && !Environment.Is64BitOperatingSystem)
                {
                    return false;
                }

                string sRegPath;
                if (IsX64 == Environment.Is64BitOperatingSystem)
                {
                    sRegPath = string.Format(@"Software\ZwSoft\{0}\2017\{1}", Name, Language);
                }
                else
                {
                    // 只有64位操作系统上的32位平台了
                    sRegPath = string.Format(@"Software\WOW6432Node\ZwSoft\{0}\2017\{1}", Name, Language);
                }

                var regKey = Registry.LocalMachine.OpenSubKey(sRegPath);
                var sLocation = regKey.GetValue("Location").ToString();

                var sPath = Path.Combine(sLocation, "zwcad.exe");
                if (File.Exists(sPath))
                {
                    return true;
                }

                return false;
            }
            catch (Exception)
            {
                return false;
            }
        }

        /// <summary>
        /// 选择字符串
        /// </summary>
        /// <returns></returns>
        public string SelString(int nLenth = 50)
        {
            return Helper.FormatLine(Desc, Key, nLenth);
        }
    }

    
}
