#include "cbase.h"

#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/QueryBox.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "ienginevgui.h"
#include <game/client/iviewport.h>

#include "tf_statsummarydialog.h"
#include "tf_mainmenu.h"
#include <convar.h>
#include "fmtstr.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFStatsSummaryDialog::CTFStatsSummaryDialog(vgui::Panel* parent, const char *panelName) : CTFDialogPanelBase(parent, panelName)
{
	Init();
}

bool CTFStatsSummaryDialog::Init()
{
	BaseClass::Init();

	m_bControlsLoaded = false;
	m_xStartLHBar = 0;
	m_xStartRHBar = 0;
	m_iBarHeight = 1;
	m_iBarMaxWidth = 1;

	m_pPlayerData = new vgui::EditablePanel(this, "statdata");
	m_pBarChartComboBoxA = new vgui::ComboBox(m_pPlayerData, "BarChartComboA", 10, false);
	m_pBarChartComboBoxB = new vgui::ComboBox(m_pPlayerData, "BarChartComboB", 10, false);
	m_pClassComboBox = new vgui::ComboBox(m_pPlayerData, "ClassCombo", 10, false);

	m_pBarChartComboBoxA->AddActionSignalTarget(this);
	m_pBarChartComboBoxB->AddActionSignalTarget(this);
	m_pClassComboBox->AddActionSignalTarget(this);

	Reset();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Applies scheme settings
//-----------------------------------------------------------------------------
void CTFStatsSummaryDialog::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	LoadControlSettings("Resource/UI/main_menu/StatsSummaryDialog.res");
	m_bControlsLoaded = true;

	// get the dimensions and position of a left-hand bar and a right-hand bar so we can do bar sizing later
	Panel *pLHBar = m_pPlayerData->FindChildByName("ClassBar1A");
	Panel *pRHBar = m_pPlayerData->FindChildByName("ClassBar1B");
	if (pLHBar && pRHBar)
	{
		int y;
		pLHBar->GetBounds(m_xStartLHBar, y, m_iBarMaxWidth, m_iBarHeight);
		pRHBar->GetBounds(m_xStartRHBar, y, m_iBarMaxWidth, m_iBarHeight);
	}

	// fill the combo box selections appropriately
	InitBarChartComboBox(m_pBarChartComboBoxA);
	InitBarChartComboBox(m_pBarChartComboBoxB);

	// fill the class names in the class combo box
	HFont hFont = scheme()->GetIScheme(GetScheme())->GetFont("ScoreboardSmall", true);
	m_pClassComboBox->SetFont(hFont);
	m_pClassComboBox->RemoveAll();
	KeyValues *pKeyValues = new KeyValues("data");
	pKeyValues->SetInt("class", TF_CLASS_UNDEFINED);
	m_pClassComboBox->AddItem("#StatSummary_Label_AsAnyClass", pKeyValues);
	for (int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass++)
	{
		pKeyValues = new KeyValues("data");
		pKeyValues->SetInt("class", iClass);
		m_pClassComboBox->AddItem(g_aPlayerClassNames[iClass], pKeyValues);
	}
	m_pClassComboBox->ActivateItemByRow(0);

	SetDefaultSelections();
	UpdateDialog();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatsSummaryDialog::PerformLayout()
{
	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: Shows this dialog as a modal dialog
//-----------------------------------------------------------------------------
void CTFStatsSummaryDialog::Show()
{
	BaseClass::Show();

	GetStatPanel()->UpdateMainMenuDialog();
	UpdateDialog();
}

//-----------------------------------------------------------------------------
// Purpose: Command handler
//-----------------------------------------------------------------------------
void CTFStatsSummaryDialog::OnCommand(const char *command)
{
	if (0 == Q_stricmp(command, "vguicancel"))
	{
		UpdateDialog();
		Hide();
		SetDefaultSelections();
	}

	BaseClass::OnCommand(command);
}

//-----------------------------------------------------------------------------
// Purpose: Resets the dialog
//-----------------------------------------------------------------------------
void CTFStatsSummaryDialog::Reset()
{
	m_aClassStats.RemoveAll();

	SetDefaultSelections();
}

//-----------------------------------------------------------------------------
// Purpose: Sets all user-controllable dialog settings to default values
//-----------------------------------------------------------------------------
void CTFStatsSummaryDialog::SetDefaultSelections()
{
	m_iSelectedClass = TF_CLASS_UNDEFINED;
	m_statBarGraph[0] = TFSTAT_POINTSSCORED;
	m_displayBarGraph[0] = SHOW_MAX;
	m_statBarGraph[1] = TFSTAT_PLAYTIME;
	m_displayBarGraph[1] = SHOW_TOTAL;

	m_pBarChartComboBoxA->ActivateItemByRow(0);
	m_pBarChartComboBoxB->ActivateItemByRow(10);
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFStatsSummaryDialog::OnKeyCodePressed(KeyCode code)
{
}

//-----------------------------------------------------------------------------
// Purpose: Sets stats to use
//-----------------------------------------------------------------------------
void CTFStatsSummaryDialog::SetStats(CUtlVector<ClassStats_t> &vecClassStats)
{
	m_aClassStats = vecClassStats;
	if (m_bControlsLoaded)
	{
		UpdateDialog();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Updates the dialog
//-----------------------------------------------------------------------------
void CTFStatsSummaryDialog::UpdateDialog()
{
	RandomSeed(Plat_MSTime());

	m_iTotalSpawns = 0;

	// if we don't have stats for any class, add empty stat entries for them 
	for (int iClass = TF_FIRST_NORMAL_CLASS; iClass <= TF_LAST_NORMAL_CLASS; iClass++)
	{
		int j;
		for (j = 0; j < m_aClassStats.Count(); j++)
		{
			if (m_aClassStats[j].iPlayerClass == iClass)
			{
				m_iTotalSpawns += m_aClassStats[j].iNumberOfRounds;
				break;
			}
		}
		if (j == m_aClassStats.Count())
		{
			ClassStats_t stats;
			stats.iPlayerClass = iClass;
			m_aClassStats.AddToTail(stats);
		}
	}


	// fill out bar charts
	UpdateBarCharts();
	// fill out class details
	UpdateClassDetails();
	// show or hide controls depending on if we're interactive or not
	UpdateControls();
}

//-----------------------------------------------------------------------------
// Purpose: Shows or hides controls
//-----------------------------------------------------------------------------
void CTFStatsSummaryDialog::UpdateControls()
{
	m_pPlayerData->SetVisible(true);
	m_pPlayerData->SetVisible(true);
}

//-----------------------------------------------------------------------------
// Purpose: Updates bar charts
//-----------------------------------------------------------------------------
void CTFStatsSummaryDialog::UpdateBarCharts()
{
	// sort the class stats by the selected stat for right-hand bar chart
	m_aClassStats.Sort(&CTFStatsSummaryDialog::CompareClassStats);

	// loop for left & right hand charts
	for (int iChart = 0; iChart < 2; iChart++)
	{
		float flMax = 0;
		for (int i = 0; i < m_aClassStats.Count(); i++)
		{
			// get max value of stat being charted so we know how to scale the graph
			float flVal = GetDisplayValue(m_aClassStats[i], m_statBarGraph[iChart], m_displayBarGraph[iChart]);
			flMax = max(flVal, flMax);
		}

		// draw the bar chart value for each player class
		for (int i = 0; i < m_aClassStats.Count(); i++)
		{
			if (0 == iChart)
			{
				// if this is the first chart, set the class label for each class
				int iClass = m_aClassStats[i].iPlayerClass;
				m_pPlayerData->SetDialogVariable(CFmtStr("class%d", i + 1), g_pVGuiLocalize->Find(g_aPlayerClassNames[iClass]));
			}
			// draw the bar for this class
			DisplayBarValue(iChart, i, m_aClassStats[i], m_statBarGraph[iChart], m_displayBarGraph[iChart], flMax);
		}
	}
}

#define MAKEFLAG(x)	( 1 << x )

#define ALL_CLASSES (1<<31)

//-----------------------------------------------------------------------------
// Purpose: Updates class details
//-----------------------------------------------------------------------------
void CTFStatsSummaryDialog::UpdateClassDetails()
{
	struct ClassDetails_t
	{
		TFStatType_t statType;			// type of stat
		int			 iFlagsClass;		// bit mask of classes to show this stat for
		const char * szResourceName;	// name of label resource
	};

	ClassDetails_t classDetails[] =
	{
		{ TFSTAT_POINTSSCORED, ALL_CLASSES, "#TF_ClassRecord_MostPoints" },
		{ TFSTAT_KILLS, ALL_CLASSES, "#TF_ClassRecord_MostKills" },
		{ TFSTAT_KILLASSISTS, ALL_CLASSES, "#TF_ClassRecord_MostAssists" },
		{ TFSTAT_CAPTURES, ALL_CLASSES, "#TF_ClassRecord_MostCaptures" },
		{ TFSTAT_DEFENSES, ALL_CLASSES, "#TF_ClassRecord_MostDefenses" },
		{ TFSTAT_DAMAGE, ALL_CLASSES, "#TF_ClassRecord_MostDamage" },
		{ TFSTAT_BUILDINGSDESTROYED, ALL_CLASSES, "#TF_ClassRecord_MostDestruction" },
		{ TFSTAT_DOMINATIONS, ALL_CLASSES, "#TF_ClassRecord_MostDominations" },
		{ TFSTAT_PLAYTIME, ALL_CLASSES, "#TF_ClassRecord_LongestLife" },
		{ TFSTAT_HEALING, MAKEFLAG(TF_CLASS_MEDIC) | MAKEFLAG(TF_CLASS_ENGINEER), "#TF_ClassRecord_MostHealing" },
		{ TFSTAT_INVULNS, MAKEFLAG(TF_CLASS_MEDIC), "#TF_ClassRecord_MostInvulns" },
		{ TFSTAT_MAXSENTRYKILLS, MAKEFLAG(TF_CLASS_ENGINEER), "#TF_ClassRecord_MostSentryKills" },
		{ TFSTAT_TELEPORTS, MAKEFLAG(TF_CLASS_ENGINEER), "#TF_ClassRecord_MostTeleports" },
		{ TFSTAT_HEADSHOTS, MAKEFLAG(TF_CLASS_SNIPER), "#TF_ClassRecord_MostHeadshots" },
		{ TFSTAT_BACKSTABS, MAKEFLAG(TF_CLASS_SPY), "#TF_ClassRecord_MostBackstabs" },
	};

	wchar_t *wzWithClassFmt = g_pVGuiLocalize->Find("#StatSummary_ScoreAsClassFmt");
	wchar_t *wzWithoutClassFmt = L"%s1";

	// display the record for each stat
	int iRow = 0;
	for (int i = 0; i < ARRAYSIZE(classDetails); i++)
	{
		TFStatType_t statType = classDetails[i].statType;

		int iClass = TF_CLASS_UNDEFINED;
		int iMaxVal = 0;

		// if there is a selected class, and if this stat should not be shown for this class, skip this stat
		if (m_iSelectedClass != TF_CLASS_UNDEFINED && (0 == (classDetails[i].iFlagsClass & MAKEFLAG(m_iSelectedClass))))
			continue;

		if (m_iSelectedClass == TF_CLASS_UNDEFINED)
		{
			// if showing best from any class, look through all player classes to determine the max value of this stat
			for (int j = 0; j < m_aClassStats.Count(); j++)
			{
				if (m_aClassStats[j].max.m_iStat[statType] > iMaxVal)
				{
					// remember max value and class that has max value
					iMaxVal = m_aClassStats[j].max.m_iStat[statType];
					iClass = m_aClassStats[j].iPlayerClass;
				}
			}
		}
		else
		{
			// show best from selected class
			iClass = m_iSelectedClass;
			for (int j = 0; j < m_aClassStats.Count(); j++)
			{
				if (m_aClassStats[j].iPlayerClass == iClass)
				{
					iMaxVal = m_aClassStats[j].max.m_iStat[statType];
					break;
				}
			}
		}

		wchar_t wzStatNum[32];
		wchar_t wzStatVal[128];
		if (TFSTAT_PLAYTIME == statType)
		{
			// playtime gets displayed as a time string
			g_pVGuiLocalize->ConvertANSIToUnicode(FormatSeconds(iMaxVal), wzStatNum, sizeof(wzStatNum));
		}
		else
		{
			// all other stats are just shown as a #
			swprintf_s(wzStatNum, ARRAYSIZE(wzStatNum), L"%d", iMaxVal);
		}

		if (TF_CLASS_UNDEFINED == m_iSelectedClass && iMaxVal > 0)
		{
			// if we are doing a cross-class view (no single selected class) and the max value is non-zero, show "# (as <class>)"
			wchar_t *wzLocalizedClassName = g_pVGuiLocalize->Find(g_aPlayerClassNames[iClass]);
			g_pVGuiLocalize->ConstructString(wzStatVal, sizeof(wzStatVal), wzWithClassFmt, 2, wzStatNum, wzLocalizedClassName);
		}
		else
		{
			// just show the value
			g_pVGuiLocalize->ConstructString(wzStatVal, sizeof(wzStatVal), wzWithoutClassFmt, 1, wzStatNum);
		}

		// set the label
		m_pPlayerData->SetDialogVariable(CFmtStr("classrecord%dlabel", iRow + 1), g_pVGuiLocalize->Find(classDetails[i].szResourceName));
		// set the value 
		m_pPlayerData->SetDialogVariable(CFmtStr("classrecord%dvalue", iRow + 1), wzStatVal);

		iRow++;
	}

	// if there are any leftover rows for the selected class, fill out the remaining rows with blank labels and values
	for (; iRow < 15; iRow++)
	{
		m_pPlayerData->SetDialogVariable(CFmtStr("classrecord%dlabel", iRow + 1), "");
		m_pPlayerData->SetDialogVariable(CFmtStr("classrecord%dvalue", iRow + 1), "");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Initializes a bar chart combo box
//-----------------------------------------------------------------------------
void CTFStatsSummaryDialog::InitBarChartComboBox(ComboBox *pComboBox)
{
	struct BarChartComboInit_t
	{
		TFStatType_t statType;
		StatDisplay_t statDisplay;
		const char *szName;
	};

	BarChartComboInit_t initData[] =
	{
		{ TFSTAT_POINTSSCORED, SHOW_MAX, "#StatSummary_StatTitle_MostPoints" },
		{ TFSTAT_POINTSSCORED, SHOW_AVG, "#StatSummary_StatTitle_AvgPoints" },
		{ TFSTAT_KILLS, SHOW_MAX, "#StatSummary_StatTitle_MostKills" },
		{ TFSTAT_KILLS, SHOW_AVG, "#StatSummary_StatTitle_AvgKills" },
		{ TFSTAT_CAPTURES, SHOW_MAX, "#StatSummary_StatTitle_MostCaptures" },
		{ TFSTAT_CAPTURES, SHOW_AVG, "#StatSummary_StatTitle_AvgCaptures" },
		{ TFSTAT_KILLASSISTS, SHOW_MAX, "#StatSummary_StatTitle_MostAssists" },
		{ TFSTAT_KILLASSISTS, SHOW_AVG, "#StatSummary_StatTitle_AvgAssists" },
		{ TFSTAT_DAMAGE, SHOW_MAX, "#StatSummary_StatTitle_MostDamage" },
		{ TFSTAT_DAMAGE, SHOW_AVG, "#StatSummary_StatTitle_AvgDamage" },
		{ TFSTAT_PLAYTIME, SHOW_TOTAL, "#StatSummary_StatTitle_TotalPlaytime" },
		{ TFSTAT_PLAYTIME, SHOW_MAX, "#StatSummary_StatTitle_LongestLife" },
	};

	// set the font
	HFont hFont = scheme()->GetIScheme(GetScheme())->GetFont("ScoreboardVerySmall", true);
	pComboBox->SetFont(hFont);
	pComboBox->RemoveAll();
	// add all the options to the combo box
	for (int i = 0; i < ARRAYSIZE(initData); i++)
	{
		KeyValues *pKeyValues = new KeyValues("data");
		pKeyValues->SetInt("stattype", initData[i].statType);
		pKeyValues->SetInt("statdisplay", initData[i].statDisplay);
		pComboBox->AddItem(g_pVGuiLocalize->Find(initData[i].szName), pKeyValues);
	}
	pComboBox->SetNumberOfEditLines(ARRAYSIZE(initData));
}

//-----------------------------------------------------------------------------
// Purpose: Helper function that sets the specified dialog variable to
//		"<value> (as <localized class name>)"
//-----------------------------------------------------------------------------
void CTFStatsSummaryDialog::SetValueAsClass(const char *pDialogVariable, int iValue, int iPlayerClass)
{
	if (iValue > 0)
	{
		wchar_t *wzScoreAsClassFmt = g_pVGuiLocalize->Find("#StatSummary_ScoreAsClassFmt");
		wchar_t *wzLocalizedClassName = g_pVGuiLocalize->Find(g_aPlayerClassNames[iPlayerClass]);
		wchar_t wzVal[16];
		wchar_t wzMsg[128];
#ifdef _WIN32 // TODO: Find a posix equivelant to _itow_s
		_itow_s(iValue, wzVal, ARRAYSIZE(wzVal), 10);
#endif
		g_pVGuiLocalize->ConstructString(wzMsg, sizeof(wzMsg), wzScoreAsClassFmt, 2, wzVal, wzLocalizedClassName);
		m_pPlayerData->SetDialogVariable(pDialogVariable, wzMsg);
	}
	else
	{
		m_pPlayerData->SetDialogVariable(pDialogVariable, "0");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the specified bar chart item to the specified value, in range 0->1
//-----------------------------------------------------------------------------
void CTFStatsSummaryDialog::DisplayBarValue(int iChart, int iBar, ClassStats_t &stats, TFStatType_t statType, StatDisplay_t statDisplay, float flMaxValue)
{
	const char *szControlSuffix = (0 == iChart ? "A" : "B");
	Panel *pBar = m_pPlayerData->FindChildByName(CFmtStr("ClassBar%d%s", iBar + 1, szControlSuffix));
	Label *pLabel = dynamic_cast<Label*>(m_pPlayerData->FindChildByName(CFmtStr("classbarlabel%d%s", iBar + 1, szControlSuffix)));
	if (!pBar || !pLabel)
		return;

	// get the stat value
	float flValue = GetDisplayValue(stats, statType, statDisplay);
	// calculate the bar size to draw, in the range of 0.0->1.0
	float flBarRange = SafeCalcFraction(flValue, flMaxValue);
	// calculate the # of pixels of bar width to draw
	int iBarWidth = max((int)(flBarRange * (float)m_iBarMaxWidth), 1);

	// Get the text label to draw for this bar.  For values of 0, draw nothing, to minimize clutter
	const char *szLabel = (flValue > 0 ? RenderValue(flValue, statType, statDisplay) : "");
	// draw the label outside the bar if there's room
	bool bLabelOutsideBar = true;
	const int iLabelSpacing = 4;
	HFont hFont = pLabel->GetFont();
	int iLabelWidth = UTIL_ComputeStringWidth(hFont, szLabel);
	if (iBarWidth + iLabelWidth + iLabelSpacing > m_iBarMaxWidth)
	{
		// if there's not room outside the bar for the label, draw it inside the bar
		bLabelOutsideBar = false;
	}

	int xBar, yBar, xLabel, yLabel;
	pBar->GetPos(xBar, yBar);
	pLabel->GetPos(xLabel, yLabel);

	m_pPlayerData->SetDialogVariable(CFmtStr("classbarlabel%d%s", iBar + 1, szControlSuffix), szLabel);
	if (1 == iChart)
	{
		// drawing code for RH bar chart
		xBar = m_xStartRHBar;
		pBar->SetBounds(xBar, yBar, iBarWidth, m_iBarHeight);
		if (bLabelOutsideBar)
		{
			pLabel->SetPos(xBar + iBarWidth + iLabelSpacing, yLabel);
		}
		else
		{
			pLabel->SetPos(xBar + iBarWidth - (iLabelWidth + iLabelSpacing), yLabel);
		}
	}
	else
	{
		// drawing code for LH bar chart
		xBar = m_xStartLHBar + m_iBarMaxWidth - iBarWidth;
		pBar->SetBounds(xBar, yBar, iBarWidth, m_iBarHeight);
		if (bLabelOutsideBar)
		{
			pLabel->SetPos(xBar - (iLabelWidth + iLabelSpacing), yLabel);
		}
		else
		{
			pLabel->SetPos(xBar + iLabelSpacing, yLabel);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Calculates a fraction and guards from divide by 0.  (Returns 0 if 
//			denominator is 0.)
//-----------------------------------------------------------------------------
float CTFStatsSummaryDialog::SafeCalcFraction(float flNumerator, float flDemoninator)
{
	if (0 == flDemoninator)
		return 0;
	return flNumerator / flDemoninator;
}

//-----------------------------------------------------------------------------
// Purpose: Formats # of seconds into a string
//-----------------------------------------------------------------------------
char *CTFStatsSummaryDialog::FormatSeconds(int seconds)
{
	static char string[64];

	int hours = 0;
	int minutes = seconds / 60;

	if (minutes > 0)
	{
		seconds -= (minutes * 60);
		hours = minutes / 60;

		if (hours > 0)
		{
			minutes -= (hours * 60);
		}
	}

	if (hours > 0)
	{
		Q_snprintf(string, sizeof(string), "%2i:%02i:%02i", hours, minutes, seconds);
	}
	else
	{
		Q_snprintf(string, sizeof(string), "%02i:%02i", minutes, seconds);
	}

	return string;
}

//-----------------------------------------------------------------------------
// Purpose: Static sort function that sorts in descending order by play time
//-----------------------------------------------------------------------------
int __cdecl CTFStatsSummaryDialog::CompareClassStats(const ClassStats_t *pStats0, const ClassStats_t *pStats1)
{	
	// sort stats first by right-hand bar graph
	TFStatType_t statTypePrimary = GET_MAINMENUPANEL(CTFStatsSummaryDialog)->m_statBarGraph[1];
	StatDisplay_t statDisplayPrimary = GET_MAINMENUPANEL(CTFStatsSummaryDialog)->m_displayBarGraph[1];
	// then by left-hand bar graph
	TFStatType_t statTypeSecondary = GET_MAINMENUPANEL(CTFStatsSummaryDialog)->m_statBarGraph[0];
	StatDisplay_t statDisplaySecondary = GET_MAINMENUPANEL(CTFStatsSummaryDialog)->m_displayBarGraph[0];

	float flValPrimary0 = GetDisplayValue((ClassStats_t &)*pStats0, statTypePrimary, statDisplayPrimary);
	float flValPrimary1 = GetDisplayValue((ClassStats_t &)*pStats1, statTypePrimary, statDisplayPrimary);
	float flValSecondary0 = GetDisplayValue((ClassStats_t &)*pStats0, statTypeSecondary, statDisplaySecondary);
	float flValSecondary1 = GetDisplayValue((ClassStats_t &)*pStats1, statTypeSecondary, statDisplaySecondary);

	// sort in descending order by primary stat value
	if (flValPrimary1 > flValPrimary0)
		return 1;
	if (flValPrimary1 < flValPrimary0)
		return -1;

	// if primary stat values are equal, sort in descending order by secondary stat value
	if (flValSecondary1 > flValSecondary0)
		return 1;
	if (flValSecondary1 < flValSecondary0)
		return -1;

	// if primary & secondary stats are equal, sort by class for consistent sort order
	return (pStats1->iPlayerClass - pStats0->iPlayerClass);
}

//-----------------------------------------------------------------------------
// Purpose: Called when text changes in combo box
//-----------------------------------------------------------------------------
void CTFStatsSummaryDialog::OnTextChanged(KeyValues *data)
{
	Panel *pPanel = reinterpret_cast<vgui::Panel *>(data->GetPtr("panel"));
	vgui::ComboBox *pComboBox = dynamic_cast<vgui::ComboBox *>(pPanel);

	if (m_pBarChartComboBoxA == pComboBox || m_pBarChartComboBoxB == pComboBox)
	{
		// a bar chart combo box changed, update the bar charts

		KeyValues *pUserDataA = m_pBarChartComboBoxA->GetActiveItemUserData();
		KeyValues *pUserDataB = m_pBarChartComboBoxB->GetActiveItemUserData();
		if (!pUserDataA || !pUserDataB)
			return;
		m_statBarGraph[0] = (TFStatType_t)pUserDataA->GetInt("stattype");
		m_displayBarGraph[0] = (StatDisplay_t)pUserDataA->GetInt("statdisplay");
		m_statBarGraph[1] = (TFStatType_t)pUserDataB->GetInt("stattype");
		m_displayBarGraph[1] = (StatDisplay_t)pUserDataB->GetInt("statdisplay");
		UpdateBarCharts();
	}
	else if (m_pClassComboBox == pComboBox)
	{
		// the class selection combo box changed, update class details

		KeyValues *pUserData = m_pClassComboBox->GetActiveItemUserData();
		if (!pUserData)
			return;

		m_iSelectedClass = pUserData->GetInt("class", TF_CLASS_UNDEFINED);

		UpdateClassDetails();
	}

}

//-----------------------------------------------------------------------------
// Purpose: Returns the stat value for specified display type
//-----------------------------------------------------------------------------
float CTFStatsSummaryDialog::GetDisplayValue(ClassStats_t &stats, TFStatType_t statType, StatDisplay_t statDisplay)
{
	switch (statDisplay)
	{
	case SHOW_MAX:
		return stats.max.m_iStat[statType];
		break;
	case SHOW_TOTAL:
		return stats.accumulated.m_iStat[statType];
		break;
	case SHOW_AVG:
		return SafeCalcFraction(stats.accumulated.m_iStat[statType], stats.iNumberOfRounds);
		break;
	default:
		AssertOnce(false);
		return 0;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets the text representation of this value
//-----------------------------------------------------------------------------
const char *CTFStatsSummaryDialog::RenderValue(float flValue, TFStatType_t statType, StatDisplay_t statDisplay)
{
	static char szValue[64];
	if (TFSTAT_PLAYTIME == statType)
	{
		// the playtime stat is shown in seconds
		return FormatSeconds((int)flValue);
	}
	else if (SHOW_AVG == statDisplay)
	{
		// if it's an average, render as a float w/2 decimal places
		Q_snprintf(szValue, ARRAYSIZE(szValue), "%.2f", flValue);
	}
	else
	{
		// otherwise, render as an integer
		Q_snprintf(szValue, ARRAYSIZE(szValue), "%d", (int)flValue);
	}

	return szValue;
}