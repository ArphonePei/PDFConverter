using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace Install
{
    class ProductList
    {
        private Product[] m_prods =
        {
            new Product() { Key='1', Name="zwcad", Desc="中望CAD 2017(x86)", Language="zh-CN", IsX64=false, FileName="PDFConverter.zrx" },
            new Product() { Key='2', Name="zwcada", Desc="中望CAD 建筑版2017(x86)", Language="zh-CN", IsX64=false, FileName="PDFConverter.zrx" },
            new Product() { Key='3', Name="zwcadm", Desc="中望CAD 机械版2017(x86)", Language="zh-CN", IsX64=false, FileName="PDFConverter.zrx" },
            new Product() { Key='4', Name="zwcad", Desc="中望CAD 2017(x64)", Language="zh-CN", IsX64=true, FileName="PDFConverterx64.zrx" },
            new Product() { Key='5', Name="zwcada", Desc="中望CAD 建筑版2017(x64)", Language="zh-CN", IsX64=true, FileName="PDFConverterx64.zrx" },
            new Product() { Key='6', Name="zwcadm", Desc="中望CAD 机械版2017(x64)", Language="zh-CN", IsX64=true, FileName="PDFConverterx64.zrx" },
        };

        public Product[] Products { get { return m_prods; } }
        public Product[] ValidProducts
        {
            get
            {
                return Products.Where(prod => prod.Exist).ToArray();
            }
        }
    }
}
