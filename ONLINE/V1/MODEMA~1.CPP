//模式管理类的处理函数，用于管理当前已经装载了哪些模式，当前激活的模式
//创作时间：Jan.10,2001
//作者：高升

#include	"modeManager.h"

CModeManager::CModeManager()
{
	for(int i = 0; i < MAX_MODE_NUM; i++)
		m_LoadedMode[i] = 0;
	m_ActiveMode = NONE_MODE_LOADED;
}
CModeManager::~CModeManager()
{
}
//设置激活模式
short CModeManager::SetActiveMode(short activeMode)
{
	if(activeMode < MAX_MODE_NUM && activeMode >= 0)
	{
		m_ActiveMode = activeMode;
		return 0;  //成功设置
	}
	else
		return 1; //模式代码不在范围内
	
}
short	CModeManager::GetActiveMode()
{
	return m_ActiveMode;
}

//标记模式已经装载
short CModeManager::SetLoadedMode(short loadMode)
{
	if(loadMode < MAX_MODE_NUM && loadMode >= 0)
	{
		m_LoadedMode[loadMode] = 1;
		return 0;  //成功设置
	}
	else
		return 1; //模式代码不在范围内
}

short CModeManager::ResetMode(short loadMode)
{
	if(loadMode < MAX_MODE_NUM && loadMode >= 0)
	{
		m_LoadedMode[loadMode] = 0;
		return 0;  //成功设置
	}
	else
		return 1; //模式代码不在范围内
}

//检查某个模式是否已经装载
short CModeManager::CheckModeLoaded(short mode)
{
	if(mode >= 0 && mode < MAX_MODE_NUM)
	{
		if(m_LoadedMode[mode])
			return 0;  //已经装载
		else
			return 1; //没有装载
	}
	else
		return 2;//超出取值范围
}