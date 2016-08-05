using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Install
{
    class Helper
    {

        public static string FillLine(string s, int nLenth = 70)
        {
            return PadRightEx(s, nLenth, ' ');
        }

        public static string PadRightEx(string str, int totalByteCount, char c)
        {
            Encoding coding = Encoding.GetEncoding("gb2312");
            int dcount = 0;
            foreach (char ch in str.ToCharArray())
            {
                if (coding.GetByteCount(ch.ToString()) == 2)
                    dcount++;
            }
            string w = str.PadRight(totalByteCount - dcount, c);
            return w;
        }

        public static string FormatLine(string sCaption, char cKey, int nLenth = 50)
        {
            return FormatLine(sCaption, cKey.ToString(), nLenth);
        }

        public static string FormatLine(string sCaption, string sKey, int nLenth = 50)
        {
            if (nLenth < 0)
            {
                nLenth = 50;
            }

            string str = PadRightEx(sCaption, nLenth - sKey.Length - 2, '.');
            str += '<' + sKey + '>';

            return str;
        }

        public static void ExitAnyKey()
        {
            Console.Write("按任意键退出");
            Console.ReadKey();
        }

        public static void OpenUrl(string surl)
        {
            System.Diagnostics.Process.Start(surl);
        }
    }
}
