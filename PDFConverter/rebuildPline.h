#pragma once
#include <vector>
using namespace std;

//给所有的点编号，多次出现的点只编一个号，后面用编号来代替点，简化过程
struct PointNum
{
	AcGePoint3d pt; //点
	int nNum;  //编号
};

//相连的点，用编号与编号连表示
struct NumMap
{
	int nNum1;  //编号
	int nNum2;  //编号
	double dBulge; //凸度
};

//绘制点
struct DrawPoint
{
	int nNum;  //编号
	double dBulge; //到下一点的凸度
};

class UserHelp
{
public:
	UserHelp(void);
public:
	~UserHelp(void);
private:
	//-----------------------------------------------------------------------------
	//Summary:找出与这一点相连的下一点，以及与之相连的凸度
	//Par:
	// nm   相连信息数组
	// nThisNum 点编号
	// dThisBulge 凸度
	// nNextNum 点编号(查找不成功时，为-1)
	//Return:
	// 查找成功为true，查找不成功为false
	//-----------------------------------------------------------------------------
	static bool FindNextPointNum(vector<NumMap>& nm, int nThisNum, double& dThisBulge, int& nNextNum);

	//-----------------------------------------------------------------------------
	//Summary:通过编号查找相应的点
	//Par:
	// vecPt2Num 点编号数组 
	// dThisBulge 凸度
	// ptThis  对应的点
	//Return:
	// 查找成功为true，查找不成功为false
	//-----------------------------------------------------------------------------
	static bool NumToPoint(vector<PointNum>& vecPt2Num, int nThisNum, AcGePoint3d& ptThis);

	//-----------------------------------------------------------------------------
	//Summary:将实体添加到数据库中
	//Par:
	// pEnt 实体指针  
	//Return:
	// 返回添加实体ID号
	//-----------------------------------------------------------------------------
	static AcDbObjectId Append(AcDbEntity* pEnt);

	//-----------------------------------------------------------------------------
	//Summary:计算圆弧的凸度
	//Par:
	// pArc 圆弧指针  
	//Return:
	// 返回计算的结果
	//-----------------------------------------------------------------------------
	static double GetBulge(AcDbArc*& pArc);

	//-----------------------------------------------------------------------------
	//Summary:得到某两点间的绘制信息，包括点编号，连接信息
	//Par:
	// vecPt2Num 点编号数组 
	// vecNumMap 连接信息数组
	// nCodeNum 需要编号的第几个点
	// ptStart  起点
	// ptEnd  终点
	// dBulge  ptStart到ptEnd的凸度
	//Return:
	// 无。
	//-----------------------------------------------------------------------------
	static void GetDrawInfo(vector<PointNum>& vecPt2Num, vector<NumMap>& vecNumMap, int& nCodeNum,
		const AcGePoint3d& ptStart, const AcGePoint3d ptEnd, double dBulge);

	static double m_cPI2;
	//-----------------------------------------------------------------------------
	//Summary:将直线，圆弧，多段线转化为(合并)多段线
	//Par:
	// idObjArr 直线，圆弧，多段线的id 
	// idPolyline 转化后多段线的ID
	//Return:
	// 转换成功为true，不成功为false。
	//-----------------------------------------------------------------------------

private:
	static bool ChangeToPolyLine(std::vector<AcDbPolyline*>& arrPtr, AcDbDatabase* pDb);

public:
	static void AppendPlines(std::vector<AcDbPolyline*>& arrPtr, AcDbDatabase* pDb);

};
