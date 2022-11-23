#include<iostream>
#include<fstream>
#include"stdio.h"
#include"cJSON.h"
#include"cJSON.c"
using namespace std;
#define IHALT 0
#define INOP 1
#define IRRMOVQ 2
#define IIRMOVQ 3
#define IRMMOVQ 4
#define IMRMOVQ 5
#define IOPL 6
#define IJXX 7
#define ICALL 8
#define IRET 9
#define IPUSHQ 0xa
#define IPOPQ 0xb
#define ALUADD 0
#define RSP 4
#define SAOK 1
#define SADR 2
#define SINS 3
#define SHLT 4 
ofstream of;
union _data{//数据两种解释方式
    char ch[8];
    long long ll;
};
class Register{
    public:
        char* get_char(){return data.ch;}//读接口
        long long get_long(){return data.ll;}
        void write_char(char*c){//写接口
            for(int i=0;i<8;i++){
                data.ch[i]=c[i];
            }
        };
        void write_long(long long l){data.ll=l;}
        Register(){data.ll=0;}
    private:
        _data data;
}; 
class Registers{
    public:
        char*get_char(int i){
            return r[i].get_char();
        }
        long long get_long(int i){
            if(i==0xf)
                return 0;
            return r[i].get_long();
        }
        void write_char(int i,char*c){
            r[i].write_char(c);
        }
        void write_long(int i,long long l){
            r[i].write_long(l);
        }
    private:
        Register r[15];
};
class cpu{
    public:
        void run(string);
        cpu():registers(){
            memset(memory,0,80000);
            PC=0;
            instr_valid=0;
            imem_error=0;
            dmem_error=0;
            state=SAOK;
            of.open("1.txt",ios::trunc);
        }
    //private:
        Registers registers;
        char memory[80000];//内存
        long long PC;
        char icode;
        char ifun;
        char rA;
        char rB;
        _data valP;
        _data valC;
        _data valA;
        long long valB;
        long long valE;
        _data valM;
        bool Cnd;
        bool ZF;
        bool SF;
        bool OF;
        char state;
        bool instr_valid;
        bool imem_error;
        bool dmem_error;
        void input_file(string);
        void Fetch();
        void Decode();
        void Execute();
        void Memory();
        void Write_back();
        void PC_update();
        void print();
        long long ALU(long long valA,long long valB,char ifun);
};
void cpu::run(string file){
    input_file(file);
    PC=0;
    while(state==SAOK){
        Fetch();
        Decode();
        Execute();
        Memory();
        Write_back();
        PC_update();
        print();
        //of<<hex<<PC<<'\t'<<registers.get_long(RSP)+8<<'\t'<<(int)memory[registers.get_long(RSP)+8]<<'\t'<<registers.get_long(6)<<endl;
    }
}
void cpu::input_file(string file){//将file中数据读入memory
    ifstream in;
    in.open(file);
    if(!in){
        cout<<1;
        return;
    }
    string s;//按行读入
    while(getline(in,s)){
        int i;
        int k=0;
        for(i=0;i<(int)s.length();i++){
            if(s[i]>='0'&&s[i]<='9'){
                k=k*16+s[i]-'0';
            }
            else if(s[i]>='a'&&s[i]<='f'){
                k=k*16+s[i]-'a'+10;
            }
            if(s[i]==':')
                break;
        }
        i+=2;
        int j=i;
        for(;j<(int)s.length();j++){
            char temp;
            if(s[j]>='0'&&s[j]<='9'){
                temp=s[j]-'0';
            }
            else if(s[j]>='a'&&s[j]<='f'){
                temp=s[j]-'a'+10;
            }
            else if(s[j]==' '){
                break;
            }
            if((j-i)%2){
                memory[k]&=(temp|0xf0);
                //cout<<hex<<k<<':'<<(int)memory[k]<<endl;
                k++;
            }
            else{
                memory[k]=(temp<<4)|0x0f;
            }
        }
        // for(;j<(int)s.length();j++){
        //     if(s[j]=='|'){
        //         j+=2;
        //         char stack[]="stack";
        //         int idx;
        //         for(idx=j;idx<j+5;idx++){
        //             if(s[idx]!=stack[idx-j]){
        //                 break;
        //             }
        //         }
        //         if(idx==j+5){
        //             registers.write_long(RSP,k);
        //         }
        //         break;
        //     }
        // }
    }
}
void cpu::Fetch(){
    if(PC>=80000){
        imem_error=1;
        return;
    }
    icode=(memory[PC]>>4)&0x0f;
    ifun=memory[PC]&0x0f;
    valP.ll=PC+1;
    if(icode>0xb||icode<0){
        instr_valid=1;
        return;
    }
    switch(icode){
        case IRRMOVQ:
        case IOPL:
        case IPUSHQ:
        case IPOPQ:
            if(PC+1>=80000){
                imem_error=1;
            }
            rA=(memory[PC+1]>>4)&0x0f;
            rB=memory[PC+1]&0x0f;
            valP.ll=PC+2;
            break;
        case IIRMOVQ:
        case IRMMOVQ:
        case IMRMOVQ:
            if(PC+9>=80000){
                instr_valid=1;
            }
            rA=(memory[PC+1]>>4)&0x0f;
            rB=memory[PC+1]&0x0f;
            for(int i=0;i<8;i++){
                valC.ch[i]=memory[PC+2+i];
            }
            valP.ll=PC+10;
            break;
        case IJXX:
        case ICALL:
            if(PC+8>=80000){
                imem_error=1;
            }
            for(int i=0;i<8;i++){
                valC.ch[i]=memory[PC+1+i];
            }
            valP.ll=PC+9;
            break;
        default:
            break;
    }
}
void cpu::Decode(){
    if(icode==IRET||icode==IPOPQ){
        valA.ll=registers.get_long(RSP);
    }
    else{
        valA.ll=registers.get_long(rA);
    }
    if(icode==ICALL||icode==IRET||icode==IPUSHQ||icode==IPOPQ){
        valB=registers.get_long(RSP);
    }
    else{
        valB=registers.get_long(rB);
    }
}
long long cpu::ALU(long long valA,long long valB,char ifun){
    long long e;
    switch(ifun){
        case 0:
            e=valB+valA;
            if(icode==IOPL){
                if(((valB^valA)>=0)&&((e^valB)<0)){//两数符号相同，相加结果符号不同
                    OF=1;
                }
                else{
                    OF=0;
                }
            }
            break;
        case 1:
            e=valB-valA;
            if((valB^valA)<0&&(valB^e)<0){
                OF=1;
            }
            break;
        case 2:
            e=valB&valA;
            OF=0;
            break;
        case 3:
            e=valB^valA;
            OF=0;
            break;
        default:
            return 0;
    }
    if(icode==IOPL){
        ZF=(e==0?1:0);
        SF=(e<0?1:0);
    }
    return e;
}
void cpu::Execute(){
    switch(icode){
        case IOPL:
            valE=ALU(valA.ll,valB,ifun);
            break;
        case IIRMOVQ:
            valE=ALU(valC.ll,0,ALUADD);
            break;
        case IRMMOVQ:
        case IMRMOVQ:
            valE=ALU(valC.ll,valB,ALUADD);
            break;
        case ICALL:
        case IPUSHQ:
            valE=ALU(-8,valB,ALUADD);
            break;
        case IRET:
        case IPOPQ:
            valE=ALU(8,valB,ALUADD);
            break;
        case IRRMOVQ:
            valE=ALU(valA.ll,0,ALUADD);
        case IJXX:
            switch(ifun){
                case 0:
                    Cnd=1;
                    break;
                case 1:
                    Cnd=(SF^OF)|ZF;
                    break;
                case 2:
                    Cnd=SF^OF;
                    break;
                case 3:
                    Cnd=ZF;
                    break;
                case 4:
                    Cnd=!ZF;
                    break;
                case 5:
                    Cnd=!(SF^OF);
                    break;
                case 6:
                    Cnd=(!(SF^OF))&(!ZF);
                default:
                    return;
            }
        default:
            break;
    }
}
void cpu::Memory(){
    switch(icode){
        case ICALL:
            if(valE+7>=80000){
                dmem_error=1;
            }
            for(int i=0;i<8;i++){
                memory[valE+i]=valP.ch[i];
            }
            break;
        case IRMMOVQ:
        case IPUSHQ:
            if(valE+7>=80000){
                dmem_error=1;
            }
            for(int i=0;i<8;i++){
                memory[valE+i]=valA.ch[i];
            }
            break;
        case IRET:
        case IPOPQ:
            if(valE+7>=80000){
                dmem_error=1;
            }
            for(int i=0;i<8;i++){
                valM.ch[i]=memory[valA.ll+i];
            }
            break;
        case IMRMOVQ:
            if(valE+7>=80000){
                dmem_error=1;
            }
            for(int i=0;i<8;i++){
                valM.ch[i]=memory[valE+i];
            }
            break;
        default:
            break;
    }
    if(icode==0){
        state=2;
    }
    else if(dmem_error|imem_error){
        state=3;
    }
    else if(instr_valid){
        state=4;
    }
}
void cpu::Write_back(){
    switch(icode){
        case IRRMOVQ:
            if(!Cnd){
                break;
            }
        case IIRMOVQ:
        case IOPL:
            registers.write_long(rB,valE);
            break;
        case IMRMOVQ:
            registers.write_long(rA,valM.ll);
            break;
        case IPOPQ:
            registers.write_long(rA,valM.ll);
        case ICALL:
        case IRET:
        case IPUSHQ:
            registers.write_long(RSP,valE);
            break;
        default:
            return;
    }
}
void cpu::PC_update(){
    switch(icode){
        case 1:
        case IRRMOVQ:
        case IIRMOVQ:
        case IRMMOVQ:
        case IMRMOVQ:
        case IOPL:
        case IPUSHQ:
        case IPOPQ:
            PC=valP.ll;
            break;
        case IJXX:
            PC=Cnd?valC.ll:valP.ll;
            break;
        case ICALL:
            PC=valC.ll;
            break;
        case IRET:
            PC=valM.ll;
            break;
        default:
            break;
    }
}
void cpu::print(){
    cJSON *REG=cJSON_CreateObject();
    cJSON_AddItemToObject(REG,"rax",cJSON_CreateNumber(registers.get_long(0)));
    cJSON_AddItemToObject(REG,"rcx",cJSON_CreateNumber(registers.get_long(1)));
    cJSON_AddItemToObject(REG,"rdx",cJSON_CreateNumber(registers.get_long(2)));
    cJSON_AddItemToObject(REG,"rbx",cJSON_CreateNumber(registers.get_long(3)));
    cJSON_AddItemToObject(REG,"rsp",cJSON_CreateNumber(registers.get_long(4)));
    cJSON_AddItemToObject(REG,"rbp",cJSON_CreateNumber(registers.get_long(5)));
    cJSON_AddItemToObject(REG,"rsi",cJSON_CreateNumber(registers.get_long(6)));
    cJSON_AddItemToObject(REG,"rdi",cJSON_CreateNumber(registers.get_long(7)));
    cJSON_AddItemToObject(REG,"r8",cJSON_CreateNumber(registers.get_long(8)));
    cJSON_AddItemToObject(REG,"r9",cJSON_CreateNumber(registers.get_long(9)));
    cJSON_AddItemToObject(REG,"r10",cJSON_CreateNumber(registers.get_long(10)));
    cJSON_AddItemToObject(REG,"r11",cJSON_CreateNumber(registers.get_long(11)));
    cJSON_AddItemToObject(REG,"r12",cJSON_CreateNumber(registers.get_long(12)));
    cJSON_AddItemToObject(REG,"r13",cJSON_CreateNumber(registers.get_long(13)));
    cJSON_AddItemToObject(REG,"r14",cJSON_CreateNumber(registers.get_long(14)));
    cJSON *CC=cJSON_CreateObject();
    cJSON_AddItemToObject(CC,"ZF",cJSON_CreateNumber(ZF));
    cJSON_AddItemToObject(CC,"SF",cJSON_CreateNumber(SF));
    cJSON_AddItemToObject(CC,"OF",cJSON_CreateNumber(OF));
    cJSON *MEM=cJSON_CreateObject();
    for(int j=0;j<80000;j+=8){
        _data m;
        m.ll=0;
        for(int i=0;i<8;i++){
            m.ch[i]=memory[j+i];
        }
        if(m.ll!=0){
            char str[5]=" ";
            sprintf(str,"%d",j);
            cJSON_AddItemToObject(MEM,str,cJSON_CreateNumber(m.ll));
        }
    }
    cJSON *root=cJSON_CreateObject();
    cJSON_AddItemToObject(root,"PC",cJSON_CreateNumber(PC));
    cJSON_AddItemToObject(root,"REG",REG);
    cJSON_AddItemToObject(root,"CC",CC);
    cJSON_AddItemToObject(root,"STAT",cJSON_CreateNumber((int)state));
    cJSON_AddItemToObject(root,"MEM",MEM);
    char *cjValue=cJSON_Print(root);
    of<<cjValue<<endl;
    free(cjValue);
    cJSON_Delete(root);
}
int main(){
    cpu _cpu;
    _cpu.run("/Users/xuboya/Desktop/c++/vscode/pj/test/pushquestion.yo");
    return 0;
}