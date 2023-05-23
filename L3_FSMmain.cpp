#include "L3_FSMevent.h"
#include "L3_msg.h"
#include "L3_timer.h"
#include "L3_LLinterface.h"
#include "protocol_parameters.h"
#include "mbed.h"


//FSM state -------------------------------------------------
//#define L3STATE_IDLE                0
#define Collect_participants        1 
#define Question_transmission       2
#define Answer_transmission         3

//state variables
static uint8_t main_state = L3STATE_IDLE; //protocol state
static uint8_t prev_state = main_state;

//parameter
uint8_t partiID[2];

//SDU (input)
static uint8_t originalWord[1030];
static uint8_t wordLen=0;

static uint8_t sdu[1030];

//serial port interface
static Serial pc(USBTX, USBRX);
static uint8_t myDestId;

//application event handler : generating SDU from keyboard input
static void L3service_processInputWord(void)
{
    char c = pc.getc();
    if (!L3_event_checkEventFlag(L3_event_dataToSend))
    {
        if (c == '\n' || c == '\r')
        {
            originalWord[wordLen++] = '\0';
            L3_event_setEventFlag(L3_event_dataToSend);
            debug_if(DBGMSG_L3,"word is ready! ::: %s\n", originalWord);
        }
        else
        {
            originalWord[wordLen++] = c;
            #if 0
            if (wordLen >= L3_MAXDATASIZE-1)
            {
                originalWord[wordLen++] = '\0';
                L3_event_setEventFlag(L3_event_dataToSend);
                pc.printf("\n max reached! word forced to be ready :::: %s\n", originalWord);
            }
            #endif
        }
    }
}



void L3_initFSM(uint8_t destId)
{

    myDestId = destId;
    //initialize service layer
    pc.attach(&L3service_processInputWord, Serial::RxIrq);

    pc.printf("Give a word to send : ");
}

void L3_FSMrun(void)
{   
    if (prev_state != main_state)
    {
        debug_if(DBGMSG_L3, "[L3] State transition from %i to %i\n", prev_state, main_state);
        prev_state = main_state;
    }

    //FSM should be implemented here! ---->>>>
    switch (main_state)
    {
        // case L3STATE_IDLE: //IDLE state description
            
        //     if (L3_event_checkEventFlag(L3_event_msgRcvd)) //if data reception event happens
        //     {
        //         //Retrieving data info.
        //         uint8_t* dataPtr = L3_LLI_getMsgPtr();
        //         uint8_t size = L3_LLI_getSize();

        //         debug("\n -------------------------------------------------\nRCVD MSG : %s (length:%i)\n -------------------------------------------------\n", 
        //                     dataPtr, size);
                
        //         pc.printf("Give a word to send : ");
                
        //         L3_event_clearEventFlag(L3_event_msgRcvd);
        //     }
        //     else if (L3_event_checkEventFlag(L3_event_dataToSend)) //if data needs to be sent (keyboard input)
        //     {
        //         //msg header setting
        //         strcpy((char*)sdu, (char*)originalWord);
        //         debug("[L3] msg length : %i\n", wordLen);
        //         //L3_LLI_dataReqFunc(sdu, wordLen, myDestId);

        //         debug_if(DBGMSG_L3, "[L3] sending msg....\n");
        //         wordLen = 0;

        //         pc.printf("Give a word to send : ");

        //         L3_event_clearEventFlag(L3_event_dataToSend);
        //     }
        //     break;
        case Collect_participants:
            if (L3_event_checkEventFlag(L3_event_quizstartPDUSend))
            {
                main_state = Collect_participants;
                L3_event_clearEventFlag(L3_event_quizstartPDUSend);
            }
            else if(L3_event_checkEventFlag(L3_event_demandPDURcvd))
            {   
                partiID[partiNum++] = L3_LLI_getSrcId();
                if (partiNum >= 2)
                {
                    // Collect PDU 전송 // "collect complement" 질문
                    //encode PDU
                    //L3_LLI_dataReqFunc(sdu, wordLen, myDestId); sdu에 pdu 들어가도 되는지 가능
                    main_state = Question_transmission;
                    L3_event_clearEventFlag(L3_event_demandPDURcvd);
                }
                else
                    main_state = Collect_participants;
                    L3_event_clearEventFlag(L3_event_demandPDURcvd);
                //}
                //else
                break;
            }
            // #if 0
            // else if(L3_event_checkEventFlag(L3_event_ComPartiCollect))
            // {
            //         main_state = Question_transmission;
            //         L3_event_clearEventFlag(L3_event_ComPartiCollect);
            //         L3_event_setEventFlag(L3_event_QuestionStart)
            // }
            // #endif
        break;
        case Question_transmission:
            if (L3_event_checkEventFlag(L3_event_questSDUinput) && L3_event_checkEventFlag(L3_event_answerIDinput)) // Quest 와 ID입력 
            {
                L3_timer_startTimer(); 
                main_state = Answer_transmission;
                L3_event_clearEventFlag(L3_event_questSDUinput && L3_event_answerIDinput) // 안 되면 나눠서 하기

            }
            break;
        break;
        case Answer_transmission:
            if(L3_event_checkEventFlag(L3_event_answerPDU))
            {
                //답변 저장 후 전송, timerstop
                Answer = L3_LLI_getMsgPtr();
                Srcid = L3_LLI_getSrcId();
                Size = L3_LLI_getSize();
                //L3_LLI_dataReqFunc(answer+Srcid , Size, Parti[]); sdu에 pdu 들어가도 되는지 가능
                L3_timer_stopTimer();
                main_state = Collect_participants;
                L3_event_clearEventFlag(L3_event_answerPDU);
            }
            else if (L3_event_checkEventFlag(L3_event_timeout))
            {
                //답변이 없습니다. PDU 전송
                //L3_LLI_dataReqFunc(답변이 없습니다.+Srcid , Size, Parti[]); sdu에 pdu 들어가도 되는지 가능
            }
            break;
        break;
        default :
            break;
    }
}