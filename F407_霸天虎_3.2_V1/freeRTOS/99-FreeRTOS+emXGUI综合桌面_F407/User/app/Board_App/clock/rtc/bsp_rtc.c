#include  ".\clock\rtc\bsp_rtc.h"

///* ���жϱ�־���������ж�ʱ��1����ʱ�䱻ˢ��֮����0 */
//__IO uint32_t TimeDisplay = 0;

/*���ڣ���Ф������ASCII��*/
uint8_t const *WEEK_STR[] = {"��", "һ", "��", "��", "��", "��", "��"};
uint8_t const *zodiac_sign[] = {"��", "��", "ţ", "��", "��", "��", "��", "��", "��", "��", "��", "��"};


/*
 * ��������NVIC_Configuration
 * ����  ������RTC���жϵ����ж����ȼ�Ϊ1�������ȼ�Ϊ0
 * ����  ����
 * ���  ����
 * ����  ���ⲿ����
 */
void RTC_NVIC_Config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;
	
	/* Configure one bit for preemption priority */
	
	/* Enable the RTC Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = RTC_WKUP_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}




static void Delay(uint16_t time)
{
  uint16_t i;
  while(--time)
  {
    for(i=0;i<2000;i++);
  }
}


static uint8_t RTC_Config(uint8_t flag)
{  
  uint32_t count=0;
  /* Enable the PWR clock */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

  /* Allow access to RTC */
  PWR_BackupAccessCmd(ENABLE);

  if(flag==0) /* The RTC Clock may varies due to LSI frequency dispersion. */
  {    
    /* Enable the LSI OSC */ 
    RCC_LSICmd(ENABLE);

    count=0;
    /* Wait till LSI is ready */  
    while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET)
    {
      Delay(1000);
      count++;
      if(count>4000)return 1;
    }

    /* Select the RTC Clock Source */
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
  }
  else /* LSE used as RTC source clock */
  {
    /* Enable the LSE OSC */
    RCC_LSEConfig(RCC_LSE_ON);

    count=0;
    /* Wait till LSE is ready */  
    while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET)
    {
      Delay(100);
      count++;
      if(count>4000)return 1;
    }

    /* Select the RTC Clock Source */
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);      
  }
  
  /* Enable the RTC Clock */
  RCC_RTCCLKCmd(ENABLE);

  /* Wait for RTC APB registers synchronisation */
  RTC_WaitForSynchro();
   
  return 0;
}



/*
 * ��������RTC_CheckAndConfig
 * ����  ����鲢����RTC
 * ����  �����ڶ�ȡRTCʱ��Ľṹ��ָ��
 * ���  ����
 * ����  ���ⲿ����
 */
uint8_t RTC_CheckAndConfig(RTC_TIME *rtc_time,uint8_t flag)
{
  RTC_InitTypeDef RTC_InitStructure;
  
  /* RTC configuration  */
  if(RTC_Config(flag)==1)return 1;
  
  if (RTC_ReadBackupRegister(RTC_BKP_DR0) != 0x32F2)
  {      
    
    /* Configure the RTC data register and RTC prescaler */
    RTC_InitStructure.RTC_AsynchPrediv = 0x7F;
    RTC_InitStructure.RTC_SynchPrediv  = 0xFF;  /* (32.768kHz / 128) - 1 = 0xFF*/
    RTC_InitStructure.RTC_HourFormat = RTC_HourFormat_24;
   
    /* Check on RTC init */
    if (RTC_Init(&RTC_InitStructure) == ERROR)
    {
      printf("\n\r        /!\\***** RTC Prescaler Config failed ********/!\\ \r\n");
    }

    /* Configure the time register */
    if(RTC_SetTime(RTC_Format_BIN, &rtc_time->RTC_Time) == ERROR)
    {
      printf("\r\n>> !! RTC Set Time failed. !! <<\r\n");
      return 1;
    } 
    else
    {
      printf("\r\n>> !! RTC Set Time success. !! <<\r\n");
      /* Indicator for the RTC configuration */
      RTC_WriteBackupRegister(RTC_BKP_DR0, 0x32F2);
    } 
      /* Configure the RTC date register */
    if(RTC_SetDate(RTC_Format_BIN, &rtc_time->RTC_Date) == ERROR)
    {
      printf("\r\n>> !! RTC Set Date failed. !! <<\r\n");
      return 1;
    } 
    else
    {
      printf("\r\n>> !! RTC Set Date success. !! <<\r\n");
      /* Indicator for the RTC configuration */
      RTC_WriteBackupRegister(RTC_BKP_DR0, 0x32F2);
    }
  }
  else
  {
    /* Check if the Power On Reset flag is set */
    if (RCC_GetFlagStatus(RCC_FLAG_PORRST) != RESET)
    {
      printf("\r\n Power On Reset occurred....\r\n");
    }
    /* Check if the Pin Reset flag is set */
    else if (RCC_GetFlagStatus(RCC_FLAG_PINRST) != RESET)
    {
      printf("\r\n External Reset occurred....\r\n");
    }

    printf("\r\n No need to configure RTC....\r\n");
    
    /* Enable the PWR clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);

    /* Allow access to RTC */
    PWR_BackupAccessCmd(ENABLE);

    /* Wait for RTC APB registers synchronisation */
    RTC_WaitForSynchro();


    /* Clear the RTC Alarm Flag */
    RTC_ClearFlag(RTC_FLAG_ALRAF);
  }  
  return 0;
}


/*
 * ��������Time_Regulate
 * ����  �������û��ڳ����ն��������ʱ��ֵ������ֵ������
 *         RTC �����Ĵ����С�
 * ����  �����ڶ�ȡRTCʱ��Ľṹ��ָ��
 * ���  ���û��ڳ����ն��������ʱ��ֵ����λΪ s
 * ����  ���ڲ�����
 */
void Time_Regulate(struct rtc_time *tm)
{
		
	tm->tm_year = 2019;	
	 
	tm->tm_mon= 11;	

	tm->tm_mday=11;	

	tm->tm_hour= 11;	    

	tm->tm_min= 11;

	tm->tm_sec= 11;

}


/*
 * ��������Time_Adjust
 * ����  ��ʱ�����
 * ����  �����ڶ�ȡRTCʱ��Ľṹ��ָ��
 * ���  ����
 * ����  ���ⲿ����
 */
void Time_Adjust(struct rtc_time *tm)
{
  /* Get time entred by the user on the hyperterminal */
//	  Time_Regulate(tm);
  
  /* RTC Configuration */
	if(RTC_Configuration()!=RTC_OK) return;
	  
  /* Get wday */
  GregorianDay(tm);
  
  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForSynchro();

  /* �޸ĵ�ǰRTC�����Ĵ������� */
  RTC_SetCounter(mktimev(tm));

  /* Wait until last write operation on RTC registers has finished */
  RTC_WaitForSynchro();
}


/*
 * ��������Time_Adjust
 * ����  ��ʱ�����
 * ����  �����ڶ�ȡRTCʱ��Ľṹ��ָ��	DecTm Ҫ�޸ĵ�Ŀ��ʱ�����ݣ�ϵͳ��ʱʹ�õġ�
				 :														SrcTmԴʱ�����ݣ��������豸���
 * ���  ����
 * ����  ���ⲿ����
 */
int8_t Time_Adjust_LCD(struct rtc_time *DecTm,struct rtc_time *SrcTm)
{
	/* RTC Configuration */
	if(RTC_Configuration()!=RTC_OK) return RTC_TIMEOUT;
	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForSynchro();

	DecTm->tm_year = SrcTm->tm_year;
	DecTm->tm_mon  = SrcTm->tm_mon;
	DecTm->tm_mday = SrcTm->tm_mday;
	DecTm->tm_hour = SrcTm->tm_hour;
	DecTm->tm_min  = SrcTm->tm_min;	
	DecTm->tm_sec  = SrcTm->tm_sec;

	//printf("\r\n %d,%d,%d  %d:%d:%d",DecTm->tm_year,DecTm->tm_mon,DecTm->tm_mday,DecTm->tm_hour,DecTm->tm_min,DecTm->tm_sec);

	/* Get wday */
	GregorianDay(DecTm);

	/* �޸ĵ�ǰRTC�����Ĵ������� */
	RTC_SetCounter(mktimev(DecTm));

	/* Wait until last write operation on RTC registers has finished */
	RTC_WaitForSynchro();

	//д���¼
	RTC_WriteBackupRegister(RTC_BKP_DR0, 0x5A5A);
	return RTC_OK;

}
#if 0
/*
 * ��������Time_Display
 * ����  ����ʾ��ǰʱ��ֵ
 * ����  ��-TimeVar RTC����ֵ����λΪ s
 * ���  ����
 * ����  ���ڲ�����
 */	
void Time_Display(uint32_t TimeVar,struct rtc_time *tm)
{
	   static uint32_t FirstDisplay = 1;
	   uint32_t BJ_TimeVar;
	   uint8_t str[15]; // �ַ����ݴ�  	

	   /*  �ѱ�׼ʱ��ת��Ϊ����ʱ��*/
	   BJ_TimeVar =TimeVar + 8*60*60;

	   to_tm(BJ_TimeVar, tm);/*�Ѷ�ʱ����ֵת��Ϊ����ʱ��*/	
	
	  if((!tm->tm_hour && !tm->tm_min && !tm->tm_sec)  || (FirstDisplay))
	  {
	      
	      GetChinaCalendar((u16)tm->tm_year, (u8)tm->tm_mon, (u8)tm->tm_mday, str);	
					printf("\r\n ����������%0.2d%0.2d,%0.2d,%0.2d", str[0], str[1], str[2],  str[3]);
	
	      GetChinaCalendarStr((u16)tm->tm_year,(u8)tm->tm_mon,(u8)tm->tm_mday,str);
					printf("\r\n ����ũ����%s\r\n", str);
	
	     if(GetJieQiStr((u16)tm->tm_year, (u8)tm->tm_mon, (u8)tm->tm_mday, str))
					printf("\r\n ����ũ����%s\r\n", str);
	
	      FirstDisplay = 0;
	  }	 	  	

	  /* ���ʱ���������ʱ�� */
	  printf(" UNIXʱ��� = %d ��ǰʱ��Ϊ: %d��(%s��) %d�� %d�� (����%s)  %0.2d:%0.2d:%0.2d\r",TimeVar,
	                    tm->tm_year, zodiac_sign[(tm->tm_year-3)%12], tm->tm_mon, tm->tm_mday, 
	                    WEEK_STR[tm->tm_wday], tm->tm_hour, 
	                    tm->tm_min, tm->tm_sec);
}
#endif

/*
*		RTC_TimeCovr ��rtcֵת���ɱ���ʱ��
*
*
*/
void RTC_TimeCovr(struct rtc_time *tm)
{
	RTC_DateTypeDef RTC_DateStructure;
	
	uint32_t BJ_TimeVar;
	
  RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);

	/*  �ѱ�׼ʱ��ת��Ϊ����ʱ��*/
//	BJ_TimeVar =	RTC_GetCounter() + 8*60*60;

	to_tm(BJ_TimeVar, tm);/*�Ѷ�ʱ����ֵת��Ϊ����ʱ��*/	

}

void RTC_WKUP_IRQHandler(void)
{
	
	if (RTC_GetITStatus(RTC_IT_WUT) != RESET)
	{
		/* Clear the RTC Second interrupt */
		RTC_ClearITPendingBit(RTC_IT_WUT);		

		/* Wait until last write operation on RTC registers has finished */
		RTC_WaitForSynchro();
	}

}

/************************END OF FILE***************************************/
