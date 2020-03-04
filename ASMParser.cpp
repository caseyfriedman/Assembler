#include "ASMParser.h"
#include <sstream>
#include <cctype>


ASMParser::ASMParser(string filename)
  // Specify a text file containing MIPS assembly instructions. Function
  // checks syntactic correctness of file and creates a list of Instructions.
{
  Instruction i;
  myFormatCorrect = true;

  myLabelAddress = 0x400000;

  int msb;

  ifstream in;
  in.open(filename.c_str());
  if(in.bad()){
    myFormatCorrect = false;
  }
  else{
    string line;
    while( getline(in, line)){
      string opcode("");
      string operand[80];
      int operand_count = 0;

      getTokens(line, opcode, operand, operand_count);
      if(opcode.length() == 0 && operand_count != 0){
	// No opcode but operands
	myFormatCorrect = false;
	break;
      }

      Opcode o = opcodes.getOpcode(opcode); //opcode is an enum i.e. a number     
      if(o == UNDEFINED){
	// invalid opcode specified
	myFormatCorrect = false;
	break;
      }

      bool success = getOperands(i, o, operand, operand_count);
      if(!success){
	myFormatCorrect = false;
	break;
      }

      string encoding = encode(i); //this is where our function is called 
      i.setEncoding(encoding);

      myInstructions.push_back(i);

    }
  }

  myIndex = 0;
}


Instruction ASMParser::getNextInstruction()
  // Iterator that returns the next Instruction in the list of Instructions.
{
  if(myIndex < (int)(myInstructions.size())){
    myIndex++;
    return myInstructions[myIndex-1];
  }
  
  Instruction i;
  return i;

}

void ASMParser::getTokens(string line,
			       string &opcode,
			       string *operand,
			       int &numOperands)
  // Decomposes a line of assembly code into strings for the opcode field and operands, 
  // checking for syntax errors and counting the number of operands.
{
    // locate the start of a comment
    string::size_type idx = line.find('#');
    if (idx != string::npos) // found a ';'
	line = line.substr(0,idx);
    int len = line.length();
    opcode = "";
    numOperands = 0;

    if (len == 0) return;
    int p = 0; // position in line

    // line.at(p) is whitespace or p >= len
    while (p < len && isWhitespace(line.at(p)))
	p++;
    // opcode starts
    while (p < len && !isWhitespace(line.at(p)))
    {
	opcode = opcode + line.at(p);
	p++;
    }
    //    for(int i = 0; i < 3; i++){
    int i = 0;
    while(p < len){
      while ( p < len && isWhitespace(line.at(p)))
	p++;

      // operand may start
      bool flag = false;
      while (p < len && !isWhitespace(line.at(p)))
	{
	  if(line.at(p) != ','){
	    operand[i] = operand[i] + line.at(p);
	    flag = true;
	    p++;
	  }
	  else{
	    p++;
	    break;
	  }
	}
      if(flag == true){
	numOperands++;
      }
      i++;
    }

    
    idx = operand[numOperands-1].find('(');
    string::size_type idx2 = operand[numOperands-1].find(')');
    
    if (idx == string::npos || idx2 == string::npos ||
	((idx2 - idx) < 2 )){ // no () found
    }
    else{ // split string
      string offset = operand[numOperands-1].substr(0,idx);
      string regStr = operand[numOperands-1].substr(idx+1, idx2-idx-1);
      
      operand[numOperands-1] = offset;
      operand[numOperands] = regStr;
      numOperands++;
    }
    
    

    // ignore anything after the whitespace after the operand
    // We could do a further look and generate an error message
    // but we'll save that for later.
    return;
}

bool ASMParser::isNumberString(string s)
  // Returns true if s represents a valid decimal integer
{
    int len = s.length();
    if (len == 0) return false;
    if ((isSign(s.at(0)) && len > 1) || isDigit(s.at(0)))
    {
	// check remaining characters
	for (int i=1; i < len; i++)
	{
	    if (!isdigit(s.at(i))) return false;
	}
	return true;
    }
    return false;
}


int ASMParser::cvtNumString2Number(string s)
  // Converts a string to an integer.  Assumes s is something like "-231" and produces -231
{
    if (!isNumberString(s))
    {
	cerr << "Non-numberic string passed to cvtNumString2Number"
		  << endl;
	return 0;
    }
    int k = 1;
    int val = 0;
    for (int i = s.length()-1; i>0; i--)
    {
	char c = s.at(i);
	val = val + k*((int)(c - '0'));
	k = k*10;
    }
    if (isSign(s.at(0)))
    {
	if (s.at(0) == '-') val = -1*val;
    }
    else
    {
	val = val + k*((int)(s.at(0) - '0'));
    }
    return val;
}
		

bool ASMParser::getOperands(Instruction &i, Opcode o,    //pretty sure the instruction coming in is blank
			    string *operand, int operand_count)
  // Given an Opcode, a string representing the operands, and the number of operands, 
  // breaks operands apart and stores fields into Instruction.
{

  if(operand_count != opcodes.numOperands(o))
    return false;

  int rs, rt, rd, imm;
  imm = 0;
  rs = rt = rd = NumRegisters;

  int rs_p = opcodes.RSposition(o);
  int rt_p = opcodes.RTposition(o);
  int rd_p = opcodes.RDposition(o);
  int imm_p = opcodes.IMMposition(o);

  if(rs_p != -1){
    rs = registers.getNum(operand[rs_p]);
    if(rs == NumRegisters)
      return false;
  }

  if(rt_p != -1){
    rt = registers.getNum(operand[rt_p]);
    if(rt == NumRegisters)
      return false;

  }
  
  if(rd_p != -1){
    rd = registers.getNum(operand[rd_p]);
    if(rd == NumRegisters)
      return false;

  }


  if(imm_p != -1){
    if(isNumberString(operand[imm_p])){  // does it have a numeric immediate field?
      imm = cvtNumString2Number(operand[imm_p]);
     
      if(((abs(imm) & 0xFFFF0000)<<1))  // too big a number to fit
	       return false;
    }
    else{ 
      if(opcodes.isIMMLabel(o)){  // Can the operand be a label?
	// Assign the immediate field an address
	imm = myLabelAddress;
  

//need to do an isnumberhex check

if((operand[imm_p].compare(0,2,"0x") == 0)){


    imm = stoi(operand[imm_p],0,16); //this is the 32 bit address




//but now we need to actually get the hex number,
  //it's in operand[imm_p] so we gotta convert it to use below
  
  int shift = imm << 2; //shift left 


  imm = (myLabelAddress + 4 & 0xF00000); // to get the most significant bits of the label (pc counter)

  imm = imm >> 20;  //to shift the unsignificant bits off

  string bits = bitset<4>(imm).to_string(); //the actual bits


  string final = bits;

  
  bits.append(bitset<28>(shift).to_string());

  

    imm = stoi(bits,0,2);




} else {


  //cout << bits << endl;
  




  long temp;

  long newlabel = bitset<32>(myLabelAddress).to_ulong();

  temp = newlabel;


  temp = temp << 2;
    
    temp = temp >> 4;

    

    imm = temp;










//need to set the immediate



	  // increment the label generator
      } myLabelAddress += 4;
    }
      else  // There is an error
	return false;
    }

  }

  i.setValues(o, rs, rt, rd, imm);

  return true;
}


string ASMParser::encode(Instruction i)
  // Given a valid instruction, returns a string representing the 32 bit MIPS binary encoding
  // of that instruction.
{
  
  string encoding = "";
  Opcode op = i.getOpcode();
  //string eachopcode = opcodes.getOpcodeField(op);
  encoding.append(opcodes.getOpcodeField(op)); // this will get the opcdoe

  string test = i.getString();




  string rs, rt, rd, funct, imm, shamt; //shamt will be 00000

  switch (opcodes.getInstType(op)) //need to handle sra
  {
    case RTYPE:        //r type means opcode + rs + rt + rd + shamt + funct
      rs = (bitset<5>(i.getRS())).to_string();
      encoding.append(rs); //this gives us a string but we want the 6 number binary 

      rt = (bitset<5>(i.getRT())).to_string();
      encoding.append(rt);


      rd = (bitset<5>(i.getRD())).to_string();
      encoding.append(rd);

      shamt = "00000";
      encoding.append(shamt);


      funct = opcodes.getFunctField(op);
      encoding.append(funct);
      break;

   case ITYPE: //opcode, rs, rt, 

      rs = (bitset<5>(i.getRS())).to_string();
      encoding.append(rs); 

      rt = (bitset<5>(i.getRT())).to_string();
      encoding.append(rt);

      if (op == SRA) //if it's sra
      {

        rd = (bitset<5>(i.getRD())).to_string();
        encoding.append(rd);

        shamt = (bitset<5>(i.getImmediate())).to_string();
        encoding.append(shamt);

        funct = opcodes.getFunctField(op);
        encoding.append(funct);
      
        break;
      }

      imm = (bitset<16>(i.getImmediate())).to_string();
      encoding.append(imm);

      break;

   case JTYPE:

  
      //cout << bitset<26>(myLabelAddress) << endl;
      //cout << i.getImmediate() << endl;
      imm = (bitset<26>(i.getImmediate())).to_string();
      encoding.append(imm);

      break;
}

  return encoding;
}
