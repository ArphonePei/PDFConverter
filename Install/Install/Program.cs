using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Install
{
    class Program
    {
        static void Main(string[] args)
        {
            var prods = new ProductList();

            // 设置颜色
            var oldBkColor = Console.BackgroundColor;
            var oldFrColor = Console.ForegroundColor;
            Console.BackgroundColor = ConsoleColor.Green;
            Console.ForegroundColor = ConsoleColor.Black;
            Console.Clear();

            // 输出
            List<string> texts = new List<string>
            {
                "*************************************************************",
                "*      本项目遵循 GNU-GPL v3 开源协议                       *",
                "*      任何人可以从以下地址获取本程序源代码：               *",
                "*      https://github.com/ArphonePei/PDFConverter           *",
                "*************************************************************",
                "",
                "",
                "*************************************************************",
                "请选择要安装PdfConverter的CAD版本:"
            };

            int nLenth = 50;
            foreach (var prod in prods.ValidProducts)
            {
                texts.Add(prod.SelString(nLenth));
            }

            texts.Add(Helper.FormatLine("退出", "Esc", nLenth));
            texts.Add(Helper.FormatLine("安装所有版本", "回车", nLenth));
            texts.AddRange(new string[]
            {
                "",
                Helper.FormatLine("访问中望CAD官方网站", '8', nLenth),
                Helper.FormatLine("访问本项目", '9', nLenth),
                "",
                "*************************************************************",
                "",
                "",
            });

            foreach (var s in texts)
            {
                Console.WriteLine(Helper.FillLine(s));
            }

            while (true)
            {
                Console.Write(("请输入要安装的版本:"));
                var keyInfo = Console.ReadKey();
                if (keyInfo.Key == ConsoleKey.Escape)   // esc
                {
                    
                    break;
                }
                else if (keyInfo.Key == ConsoleKey.Enter || keyInfo.Key == ConsoleKey.Spacebar)   // Enter
                {
                    foreach (var prod in prods.ValidProducts)
                    {
                        prod.DoInstall();
                    }

                    Helper.ExitAnyKey();
                    break;
                }
                else if (keyInfo.Modifiers != 0)
                {
                    continue;
                }

                switch (keyInfo.KeyChar)
                {
                    case '8':
                        Helper.OpenUrl("http://www.zwcad.com");
                        break;
                    case '9':
                        Helper.OpenUrl("https://github.com/ArphonePei/PDFConverter");
                        break;
                    default:
                        var sels = prods.ValidProducts.Where(prod => prod.Key == keyInfo.KeyChar);
                        if (sels.Any())
                        {
                            foreach (var prod in sels)
                            {
                                prod.DoInstall();
                            }
                        }
                        else
                        {
                            Console.WriteLine();
                            Console.WriteLine(Helper.FillLine("无效输入"));
                        }


                        break;
                }
            }


            Console.BackgroundColor = oldBkColor;
            Console.ForegroundColor = oldFrColor;

            
        }
    }
}
