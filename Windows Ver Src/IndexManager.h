#ifndef _INDEX_H_
#define _INDEX_H_
#include "BufferManager.h"
#include "DSL_miniSQL.h"
#include <stdlib.h>
#include <list>
#define POINTERLENGTH 5
extern BufferManager buf;

class IndexLeaf{//叶节点
public:
	string key; //关键字
	int offsetInFile;//文件中的位置
	int offsetInBlock;//block中的位置
	IndexLeaf():
		key(""), offsetInFile(0), offsetInBlock(0){}
	IndexLeaf(string k, int oif, int oib):
		key(k), offsetInFile(oif), offsetInBlock(oib){}
};

class IndexBranch{//not a leaf, normal node
public:
	string key;
	int ptrChild;	//block pointer,????????????ν????????????block??????е????
	IndexBranch():
		key(""), ptrChild(0){}
	IndexBranch(string k, int ptrC):
		key(k), ptrChild(ptrC){}
};

class BPlusTree{
public:
	bool isRoot;
	int bufferNum;
	int ptrFather;		//block pointer, if is root, this pointer is useless
	int recordNum;
	int columnLength;
	BPlusTree(){}
	BPlusTree(int vbufNum): bufferNum(vbufNum), recordNum(0){}
	int getPtr(int pos){//获取位置在pos处的指针
		int ptr = 0;
		for(int i = pos; i<pos + POINTERLENGTH ; i++){
			ptr = 10 * ptr + buf.GetBlockContent(bufferNum,i) - '0';
		}
		return ptr;
	}
	int getRecordNum(){//获取记录的数量
		int recordNum = 0;
		for(int i = 2; i<6; i++){
			if(buf.GetBlockContent(bufferNum,i) == EMPTY) break;
			recordNum = 10 * recordNum + buf.GetBlockContent(bufferNum,i) - '0';
		}
		return recordNum;
	}
};

//unleaf node
class Branch: public BPlusTree
{
public:
	list<IndexBranch> nodelist;
	Branch(){}
	Branch(int vbufNum): BPlusTree(vbufNum){}//this is for new added branch
	Branch(int vbufNum, const Index& indexinfor){
		bufferNum = vbufNum;
		isRoot = ( buf.bufferblocks[bufferNum].getcontent(0)== 'R' );
		int recordCount = getRecordNum();
		recordNum = 0;//recordNum will increase when function insert is called, and finally as large as recordCount
		ptrFather = getPtr(6);
		columnLength = indexinfor.columnLength;
		int position = 6 + POINTERLENGTH;
		for(int i = 0; i < recordCount; i++)
		{
			string key = "";
			for(int j = position; j < position + columnLength; j++){
				if(buf.bufferblocks[bufferNum].getcontent(j)== EMPTY) break;
				else key += buf.bufferblocks[bufferNum].getcontent(j);
			}
			position += columnLength;
			int ptrChild = getPtr(position);
			position += POINTERLENGTH;
			IndexBranch node(key, ptrChild);
			insert(node);//插入节点
		
		}
	}
	~Branch(){
		//isRoot
		if(isRoot) buf.bufferblocks[bufferNum].setcontent(0,'R');
		else buf.bufferblocks[bufferNum].setcontent(0,'_');
		//is not a Leaf
		buf.bufferblocks[bufferNum].setcontent(1,'_');
		//recordNum
		char tmpt[5];
		itoa(recordNum, tmpt, 10);
		string strRecordNum = tmpt; 	           			
		while(strRecordNum.length() < 4)
			strRecordNum = '0' + strRecordNum;
		strncpy(buf.bufferblocks[bufferNum].getcontents()+ 2, strRecordNum.c_str() , 4);
		
		//nodelist
		if(nodelist.size() == 0){
			cout << "Oh, no no no!! That's impossible." << endl;
			exit(0);
		}

		list<IndexBranch>::iterator i;
		int position = 6 + POINTERLENGTH;	//?????λ????洢index??????????
		for(i = nodelist.begin(); i!=nodelist.end(); i++)
		{
			string key = (*i).key;
			while(key.length() <columnLength)
				key += EMPTY;
			strncpy(buf.bufferblocks[bufferNum].getcontents() + position, key.c_str(), columnLength);
			position += columnLength;
			
			char tmpt[5];
			itoa((*i).ptrChild, tmpt, 10);
			string ptrChild = tmpt;
			while(ptrChild.length() < POINTERLENGTH)
				ptrChild = '0' + ptrChild;
			strncpy(buf.bufferblocks[bufferNum].getcontents() + position, ptrChild.c_str(), POINTERLENGTH);
			position += POINTERLENGTH;
		}
	}
	void insert(IndexBranch node){
		recordNum++;
		list<IndexBranch>::iterator i = nodelist.begin();
		if(nodelist.size() == 0)
			nodelist.insert(i, node);
		else{
			for(i = nodelist.begin(); i!=nodelist.end(); i++)
				if((*i).key > node.key) break;//将节点插入合适的位置
			nodelist.insert(i, node);
		}
	}
	
	IndexBranch pop(){//弹出栈顶节点
		recordNum--;
		IndexBranch tmpt = nodelist.back();
		nodelist.pop_back();
		return tmpt;
	}
	
	IndexBranch getfront(){//获取第一个节点
		return nodelist.front();
	}
};

class Leaf: public BPlusTree
{
public:	
	int nextSibling;	//block pointer
	int lastSibling;	//block pointer
	list<IndexLeaf> nodelist;
	Leaf(int vbufNum){	//this kind of leaf is added when old leaf is needed to be splited
		bufferNum = vbufNum;
		recordNum = 0;
		nextSibling = lastSibling = 0;
	}
	Leaf(int vbufNum, const Index& indexinfor){
		bufferNum = vbufNum;
		isRoot = ( buf.bufferblocks[bufferNum].getcontent(0) == 'R' );
		int recordCount = getRecordNum();
		recordNum = 0;
		ptrFather = getPtr(6);
		lastSibling = getPtr(6 + POINTERLENGTH);
		nextSibling = getPtr(6 + POINTERLENGTH*2);
		columnLength = indexinfor.columnLength;	//?????????????????????

		//cout << "recordCount = "<< recordCount << endl;
		int position = 6 + POINTERLENGTH*3;	//?????λ????洢index??????????
		for(int i = 0; i < recordCount; i++)
		{	
			string key = "";
			for(int i = position; i < position + columnLength; i++){
				if(buf.bufferblocks[bufferNum].getcontent(i) == EMPTY) break;
				else key += buf.bufferblocks[bufferNum].getcontent(i);
			}
			position += columnLength;
			//cout << "get offsetInFile" << endl;
			int offsetInFile = getPtr(position);
			//cout << "get offsetInBlock" << endl;
			position += POINTERLENGTH;
			int offsetInBlock  = getPtr(position);
			position += POINTERLENGTH;
			IndexLeaf node(key, offsetInFile, offsetInBlock);
			insert(node);
		}
	}
	~Leaf(){
		//isRoot
		if(isRoot) buf.bufferblocks[bufferNum].setcontent(0,'R');
		else buf.bufferblocks[bufferNum].setcontent(0,'_');
		//isLeaf
		buf.bufferblocks[bufferNum].setcontent(1,'L');
		//recordNum
		char tmpt[5];
		itoa(recordNum, tmpt, 10);
		string strRecordNum = tmpt;
		while(strRecordNum.length() < 4)
			strRecordNum = '0' + strRecordNum;
		int position = 2;
		strncpy(buf.bufferblocks[bufferNum].getcontents() + position, strRecordNum.c_str() , 4);
		position += 4;
		
		itoa(ptrFather, tmpt, 10);
		string strptrFather = tmpt;
		while(strptrFather.length() < POINTERLENGTH)
			strptrFather = '0' + strptrFather;
		strncpy(buf.bufferblocks[bufferNum].getcontents() + position, strptrFather.c_str() , POINTERLENGTH);
		position += POINTERLENGTH;
		
		itoa(lastSibling, tmpt, 10);
		string strLastSibling = tmpt;
		while(strLastSibling.length() < POINTERLENGTH)
			strLastSibling = '0' + strLastSibling;
		strncpy(buf.bufferblocks[bufferNum].getcontents() + position, strLastSibling.c_str() , POINTERLENGTH);
		position += POINTERLENGTH;
		
		itoa(nextSibling, tmpt, 10);
		string strNextSibling = tmpt;
		while(strNextSibling.length() < POINTERLENGTH)
			strNextSibling = '0' + strNextSibling;
		strncpy(buf.bufferblocks[bufferNum].getcontents() + position, strNextSibling.c_str() , POINTERLENGTH);
		position += POINTERLENGTH;
		
		//nodelist
		if(nodelist.size() == 0){
			cout << "Oh, no no no!! That's impossible." << endl;
			exit(0);
		}

		list<IndexLeaf>::iterator i;
		for(i = nodelist.begin(); i!=nodelist.end(); i++)
		{
			string key = (*i).key;
			while(key.length() <columnLength)
				key += EMPTY;
			strncpy(buf.bufferblocks[bufferNum].getcontents() + position, key.c_str(), columnLength);
			position += columnLength;

			itoa((*i).offsetInFile, tmpt, 10);
			string offsetInFile = tmpt;
			while(offsetInFile.length() < POINTERLENGTH)
				offsetInFile = '0' + offsetInFile;
			strncpy(buf.bufferblocks[bufferNum].getcontents() + position, offsetInFile.c_str(), POINTERLENGTH);
			position += POINTERLENGTH;

			itoa((*i).offsetInBlock, tmpt, 10);
			string offsetInBlock = tmpt;
			while(offsetInBlock.length() < POINTERLENGTH)
				offsetInBlock = '0' + offsetInBlock;
			strncpy(buf.bufferblocks[bufferNum].getcontents() + position, offsetInBlock.c_str(), POINTERLENGTH);
			position += POINTERLENGTH;
			//cout << key<< "\t" <<offsetInFile<<"\t"<< offsetInFile<< endl;
		}
	}

	void insert(IndexLeaf node){
		recordNum++;
		//cout << "onece" << endl;
		list<IndexLeaf>::iterator i = nodelist.begin();
		if(nodelist.size() == 0){
			nodelist.insert(i, node);
			return;
		}
		else{
			for(i = nodelist.begin(); i!=nodelist.end(); i++)
				if((*i).key > node.key) break;
		}
		nodelist.insert(i, node);
	}
	IndexLeaf pop(){
		recordNum--;
		IndexLeaf tmpt = nodelist.back();
		nodelist.pop_back();
		return tmpt;
	}
	IndexLeaf getfront(){//this is the smallest of all the keys of the list
		return nodelist.front();
	}
};

class IndexManager{
public:
	void createIndex(const Table& tableinfor, Index& indexinfor){
		//create a new file
		string filename = indexinfor.index_name + ".index";
		fstream  fout(filename.c_str(), ios::out);
		fout.close();
		//create a root for the tree
		int blockNum = buf.GetEmptyBlock();
		buf.bufferblocks[blockNum].filename = filename;
		buf.bufferblocks[blockNum].blockoffset = 0;
		buf.bufferblocks[blockNum].written = 1;
		buf.bufferblocks[blockNum].used = 1;
		buf.bufferblocks[blockNum].setcontent(0,'R');//block????λ????????? 
		buf.bufferblocks[blockNum].setcontent(1,'L');//block????λ????????????
		//???????λ?????????ж????????????block????????????????9999???? 
		memset( buf.bufferblocks[blockNum].getcontents() + 2, '0' , 4);//????????????? 
		//?????3*LENGTHBlockPtrλ????????????????????????
		for(int i = 0; i < 3; i++)
			memset( buf.bufferblocks[blockNum].getcontents()+6 + POINTERLENGTH*i, '0' , POINTERLENGTH);
		indexinfor.blockNum++;
		
		//retrieve datas of the table and form a B+ Tree
		filename = tableinfor.name + ".table";
		string stringrow;
		string key;

		int length = tableinfor.totalLength + 1;//增加一位判断记录是否为空
		const int recordNum = BLOCKSIZE / length;
		
		//read datas from the record and sort it into a B+ Tree and store it
		for(int blockoffset = 0; blockoffset < tableinfor.blockNum; blockoffset++){
			int bufferNum = buf.IfinBuffer(filename, blockoffset);
			if(bufferNum == -1){
				bufferNum = buf.GetEmptyBlock();
				buf.LoadBlock(filename, blockoffset, bufferNum);
			}
			for(int offset = 0; offset < recordNum; offset ++){
				int position  = offset * length;
				stringrow = buf.bufferblocks[bufferNum].getcontents(position, position + length);
				if(stringrow.c_str()[0] == EMPTY) continue;//inticate that this row of record have been deleted
				stringrow.erase(stringrow.begin());	//去掉第一位标识
				key = getColumnValue(tableinfor, indexinfor, stringrow);
				IndexLeaf node(key, blockoffset, offset);
				insertValue(indexinfor, node);//将节点插入B+树
			}
		}
	}

	IndexBranch insertValue(Index& indexinfor, IndexLeaf node, int blockoffset = 0){
		IndexBranch reBranch;//for return, intial to be empty
		string filename = indexinfor.index_name + ".index";
		int bufferNum = buf.GetBufferID(filename, blockoffset);
		bool isLeaf = ( buf.bufferblocks[bufferNum].getcontent(1) == 'L' );// L for leaf
		if(isLeaf){
			Leaf leaf(bufferNum, indexinfor);
			leaf.insert(node);

			//?ж?????????
			const int RecordLength = indexinfor.columnLength + POINTERLENGTH*2;//对应属性长度+2个block指针
			const int MaxrecordNum = (BLOCKSIZE-6-POINTERLENGTH*3)/RecordLength;//一个block中记录的数量
			if( leaf.recordNum > MaxrecordNum ){//record number is too great, need to split
				if(leaf.isRoot){//this leaf is a root
					int rbufferNum = leaf.bufferNum;	// buffer number for new root
					leaf.bufferNum = buf.AddBlock(indexinfor);	//find a new place for old leaf
					int sbufferNum = buf.AddBlock(indexinfor);	// buffer number for sibling 
					Branch branchRoot(rbufferNum);	//new root, which is branch indeed
					Leaf leafadd(sbufferNum);	//sibling
					
					//is root
					branchRoot.isRoot = 1;
					leafadd.isRoot = 0;
					leaf.isRoot = 0;

					branchRoot.ptrFather = leafadd.ptrFather = leaf.ptrFather = 0;
					branchRoot.columnLength = leafadd.columnLength = leaf.columnLength;
					//link the newly added leaf block in the link list of leaf
					leafadd.lastSibling = buf.bufferblocks[leaf.bufferNum].blockoffset;
					leaf.nextSibling = buf.bufferblocks[leafadd.bufferNum].blockoffset;
					while(leafadd.nodelist.size() < leaf.nodelist.size()){
						IndexLeaf tnode = leaf.pop();
						leafadd.insert(tnode);
					}

					IndexBranch tmptNode;
					tmptNode.key = leafadd.getfront().key;
					tmptNode.ptrChild = buf.bufferblocks[leafadd.bufferNum].blockoffset;
					branchRoot.insert(tmptNode);
					tmptNode.key = leaf.getfront().key;
					tmptNode.ptrChild = buf.bufferblocks[leaf.bufferNum].blockoffset;
					branchRoot.insert(tmptNode);
					return reBranch;
				}
				else{//this leaf is not a root, we have to cascade up
					int bufferNum = buf.AddBlock(indexinfor);
					Leaf leafadd(bufferNum);
					leafadd.isRoot = 0;
					leafadd.ptrFather = leaf.ptrFather;
					leafadd.columnLength = leaf.columnLength;
					
					//link the newly added leaf block in the link list of leaf
					leafadd.nextSibling = leaf.nextSibling;
					leafadd.lastSibling = buf.bufferblocks[leaf.bufferNum].blockoffset;
					leaf.nextSibling = buf.bufferblocks[leafadd.bufferNum].blockoffset;
					if(leafadd.nextSibling != 0){
						int bufferNum = buf.GetBufferID(filename, leafadd.nextSibling);
						Leaf leafnext(bufferNum, indexinfor);
						leafnext.lastSibling = buf.bufferblocks[leafadd.bufferNum].blockoffset;
					}
					while(leafadd.nodelist.size() < leaf.nodelist.size()){
						IndexLeaf tnode = leaf.pop();
						leafadd.insert(tnode);
					}
					reBranch.key = leafadd.getfront().key;
					reBranch.ptrChild = leaf.nextSibling;
					return reBranch;
				}
			}
			else{//not need to split,just return
				return reBranch;
			}

		}
		else{//not a leaf
			Branch branch(bufferNum, indexinfor);
			list<IndexBranch>::iterator i = branch.nodelist.begin();
			if((*i).key > node.key){	//????2???????????????С
				(*i).key = node.key;	//????????????
			}
			else{
				for(i = branch.nodelist.begin(); i != branch.nodelist.end(); i++)
					if((*i).key > node.key) break;
				i--;//???(*i) ????????λ??
			}
			IndexBranch bnode = insertValue(indexinfor, node, (*i).ptrChild);//go down
			
			if(bnode.key == ""){
				return reBranch;
			}
			else{//bnode.key != "", ??????B???????split?????????????????????
				branch.insert(bnode);
				const int RecordLength = indexinfor.columnLength + POINTERLENGTH;
				const int MaxrecordNum = (BLOCKSIZE-6-POINTERLENGTH) / RecordLength;
				if(branch.recordNum > MaxrecordNum){//need to split up
					if(branch.isRoot){
						int rbufferNum = branch.bufferNum;	// buffer number for new root
						branch.bufferNum = buf.AddBlock(indexinfor);	//find a new place for old branch
						int sbufferNum = buf.AddBlock(indexinfor);	// buffer number for sibling 
						Branch branchRoot(rbufferNum);	//new root
						Branch branchadd(sbufferNum);	//sibling

						//is root
						branchRoot.isRoot = 1;
						branchadd.isRoot = 0;
						branch.isRoot = 0;

						branchRoot.ptrFather = branchadd.ptrFather = branch.ptrFather = 0;
						branchRoot.columnLength = branchadd.columnLength = branch.columnLength;
						
						while(branchadd.nodelist.size() < branch.nodelist.size()){
							IndexBranch tnode = branch.pop();
							branchadd.insert(tnode);
						}

						IndexBranch tmptNode;
						tmptNode.key = branchadd.getfront().key;
						tmptNode.ptrChild = buf.bufferblocks[branchadd.bufferNum].blockoffset;
						branchRoot.insert(tmptNode);
						tmptNode.key = branch.getfront().key;
						tmptNode.ptrChild = buf.bufferblocks[branch.bufferNum].blockoffset;
						branchRoot.insert(tmptNode);
						return reBranch;//here the function must have already returned to the top lay
					}
					else{//branch is not a root
						int bufferNum = buf.AddBlock(indexinfor);
						Branch branchadd(bufferNum);
						branchadd.isRoot = 0;
						branchadd.ptrFather = branch.ptrFather;
						branchadd.columnLength = branch.columnLength;
						
						while(branchadd.nodelist.size() < branch.nodelist.size()){
							IndexBranch tnode = branch.pop();
							branchadd.insert(tnode);
						}
						reBranch.key = branchadd.getfront().key;
						reBranch.ptrChild = buf.bufferblocks[bufferNum].blockoffset;
						return reBranch;
					}
				}
				else{//not need to split,just return
					return reBranch;
				}
			}
		}
		return reBranch;//here is just for safe
	}
	

	Data selectEqual(const Table& tableinfor, const Index& indexinfor, string key, int blockoffset = 0){//start from the root and look down
		Data datas;
		string filename = indexinfor.index_name + ".index";
		int bufferNum = buf.GetBufferID(filename, blockoffset);
		bool isLeaf = ( buf.bufferblocks[bufferNum].getcontent(1) == 'L' );// L for leaf
		if(isLeaf){
			Leaf leaf(bufferNum, indexinfor);
			list<IndexLeaf>::iterator i = leaf.nodelist.begin();
			for(i = leaf.nodelist.begin(); i!= leaf.nodelist.end(); i++)
				if((*i).key == key){
					filename = indexinfor.table_name + ".table";
					int recordBufferNum = buf.GetBufferID(filename, (*i).offsetInFile);//???????buffer
					int position = (tableinfor.totalLength +1)* ((*i).offsetInBlock);
					string stringrow = buf.bufferblocks[recordBufferNum].getcontents(position, position + tableinfor.totalLength);
					if(stringrow.c_str()[0] != EMPTY){
						stringrow.erase(stringrow.begin());//????λ???
						Row splitedRow = splitRow(tableinfor, stringrow);
						datas.rows.push_back(splitedRow);
						return datas;
					}
				}
		}
		else{	//it is not a leaf
			Branch branch(bufferNum, indexinfor);
			list<IndexBranch>::iterator i = branch.nodelist.begin();
			for(i = branch.nodelist.begin(); i != branch.nodelist.end(); i++){
				if((*i).key > key){
					//cout << (*i).key << "==" << key << endl;
					i--;//???(*i) ????????λ??
					break;
				}
			}
			if(i == branch.nodelist.end()) i--;
			datas = selectEqual(tableinfor, indexinfor, key, (*i).ptrChild);
		}
		return datas;
	}	

	Data selectBetween(const Table& tableinfor, const Index& indexinfor, string keyFrom, string keyTo, int blockoffset = 0){
		Data datas;
		string filename = indexinfor.index_name + ".index";
		int bufferNum = buf.GetBufferID(filename, blockoffset);
		bool isLeaf = ( buf.bufferblocks[bufferNum].getcontent(1) == 'L' );// L for leaf
		if(isLeaf){
			do{
				Leaf leaf(bufferNum, indexinfor);
				list<IndexLeaf>::iterator i;
				for(i = leaf.nodelist.begin(); i!= leaf.nodelist.end(); i++){
					if((*i).key >= keyFrom){
						if((*i).key > keyTo){
							return datas;
						}
						filename = indexinfor.table_name + ".table";
						int recordBufferNum = buf.GetBufferID(filename, (*i).offsetInFile);//???????buffer
						int position = (tableinfor.totalLength +1)* ((*i).offsetInBlock);
						string stringrow = buf.bufferblocks[recordBufferNum].getcontents(position, position + tableinfor.totalLength);
						if(stringrow.c_str()[0] != EMPTY){
							stringrow.erase(stringrow.begin());//????λ???
							Row splitedRow = splitRow(tableinfor, stringrow);
							datas.rows.push_back(splitedRow);
						}
					}
				}
				if(leaf.nextSibling != 0){
					filename = indexinfor.index_name + ".index";
					bufferNum = buf.GetBufferID(filename, leaf.nextSibling);
				}
				else return datas;
			}while(1);
		}
		else{//not leaf, go down to the leaf
			Branch branch(bufferNum, indexinfor);
			list<IndexBranch>::iterator i = branch.nodelist.begin();
			if((*i).key > keyFrom){//???keyFrom ????С???????С??????????????????
				datas = selectBetween(tableinfor, indexinfor, keyFrom, keyTo, (*i).ptrChild);
				return datas;
			}
			else{//???????????????????
				for(i = branch.nodelist.begin(); i != branch.nodelist.end(); i++){
					if((*i).key > keyFrom){
						i--;//???(*i) ????????λ??
						break;
					}
				}
				datas = selectBetween(tableinfor, indexinfor, keyFrom, keyTo, (*i).ptrChild);
				return datas;
			}
		}
		return datas;
	}
private:
	Row splitRow(Table tableinfor, string row){
		Row splitedRow;
		int s_pos = 0, f_pos = 0;//start position & finish position
		for(int i= 0; i < tableinfor.attriNum; i++){
			s_pos = f_pos;
			f_pos += tableinfor.attributes[i].length;
			string col;
			for(int j = s_pos; j < f_pos; j++){
				if(row[j] == EMPTY) break;
				col += row[j];
			}
			splitedRow.columns.push_back(col);
		}
		return splitedRow;
	}

private:
	string getColumnValue(const Table& tableinfor, const Index& indexinfor, string row){
		string colValue;
		int s_pos = 0, f_pos = 0;	//start position & finish position
		for(int i= 0; i <= indexinfor.column; i++){
			s_pos = f_pos;
			f_pos += tableinfor.attributes[i].length;
		}
		for(int j = s_pos; j < f_pos && row[j] != EMPTY; j++)	colValue += row[j];
		return colValue;
	}
	

public:
	void dropIndex(Index& indexinfor){
		string filename = indexinfor.index_name + ".index";
		if( remove(filename.c_str()) != 0 )
			perror( "Error deleting file" );
		else
			buf.SetUnused(filename);
	}

/*	void dropIndex(const Table& tableinfor){
		string filename = tableinfor.name + ".index";
		if( remove(filename.c_str()) != 0 )
			return;//perror( "Error deleting file" );
		else
			buf.SetUnused(filename);
	}
*/
/*	void dropTable(Table& tableinfor){
		string filename = tableinfor.name + ".index";
		buf.SetUnused(filename);
	}
*/
	void deleteValue(){}
};

#endif
