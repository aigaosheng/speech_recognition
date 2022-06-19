//模式管理类，用于管理当前已经装载了哪些模式，当前激活的模式
//创作时间：Jan.10,2001
//作者：高升

#ifndef	_MODEMANAGER_H
#define	_MODEMANAGER_H

//工作模式
#define	MAX_MODE_NUM	3
#define	NONE_MODE_LOADED	MAX_MODE_NUM
//工作模式编码
#define	DICTATION_MODE	0
#define	COMMAND_MODE	1
#define	ADAPT_MODE		2


class CModeManager
{
public:
	CModeManager();
	~CModeManager();
private:	
	short	m_ActiveMode; //当前激活的工作模式，0：听写; 1：命令
	short	m_LoadedMode[MAX_MODE_NUM]; //0:没有装载; 1:已装载
public:
	short	SetActiveMode(short); 
	short	SetLoadedMode(short);
	short	ResetMode(short);	
	short	GetActiveMode(void);
	short	CheckModeLoaded(short);
};

#endif