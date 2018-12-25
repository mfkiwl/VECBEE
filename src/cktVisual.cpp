#include "cktVisual.h"


using namespace abc;
using namespace std;


void WriteDotNtk(Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes, Vec_Ptr_t * vNodesShow, char * pFileName, int fGateNames, int fUseReverse )
{
    FILE * pFile;
    abc::Abc_Obj_t * pNode, * pFanin;
    char * pSopString;
    int LevelMin, LevelMax, fHasCos, Level, i, k, fHasBdds, fCompl, Prev;
    int Limit = 500;

    assert( Abc_NtkIsStrash(pNtk) || Abc_NtkIsLogic(pNtk) );

    if ( vNodes->nSize < 1 )
    {
        printf( "The set has no nodes. DOT file is not written.\n" );
        return;
    }

    if ( vNodes->nSize > Limit )
    {
        printf( "The set has more than %d nodes. DOT file is not written.\n", Limit );
        return;
    }

    // start the stream
    if ( (pFile = fopen( pFileName, "w" )) == NULL )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", pFileName );
        return;
    }

    // transform logic functions from BDD to SOP
    if ( (fHasBdds = Abc_NtkIsBddLogic(pNtk)) )
    {
        if ( !Abc_NtkBddToSop(pNtk, -1, ABC_INFINITY) )
        {
            printf( "Io_WriteDotNtk(): Converting to SOPs has failed.\n" );
            return;
        }
    }

    // mark the nodes from the set
    Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
        pNode->fMarkC = 1;
    if ( vNodesShow )
        Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodesShow, pNode, i )
            pNode->fMarkB = 1;

    // get the levels of nodes
    LevelMax = Abc_NtkLevel( pNtk );
    if ( fUseReverse )
    {
        LevelMin = Abc_NtkLevelReverse( pNtk );
        assert( LevelMax == LevelMin );
        Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
            if ( Abc_ObjIsNode(pNode) )
                pNode->Level = LevelMax - pNode->Level + 1;
    }

    // find the largest and the smallest levels
    LevelMin = 10000;
    LevelMax = -1;
    fHasCos  = 0;
    Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
    {
        if ( Abc_ObjIsCo(pNode) )
        {
            fHasCos = 1;
            continue;
        }
        if ( LevelMin > (int)pNode->Level )
            LevelMin = pNode->Level;
        if ( LevelMax < (int)pNode->Level )
            LevelMax = pNode->Level;
    }

    // set the level of the CO nodes
    if ( fHasCos )
    {
        LevelMax++;
        Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
        {
            if ( Abc_ObjIsCo(pNode) )
                pNode->Level = LevelMax;
        }
    }

    // write the DOT header
    fprintf( pFile, "# %s\n",  "Network structure generated by ABC" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "digraph network {\n" );
    fprintf( pFile, "size = \"7.5,10\";\n" );
//    fprintf( pFile, "size = \"10,8.5\";\n" );
//    fprintf( pFile, "size = \"14,11\";\n" );
//    fprintf( pFile, "page = \"8,11\";\n" );
//  fprintf( pFile, "ranksep = 0.5;\n" );
//  fprintf( pFile, "nodesep = 0.5;\n" );
    fprintf( pFile, "center = true;\n" );
//    fprintf( pFile, "orientation = landscape;\n" );
//  fprintf( pFile, "edge [fontsize = 10];\n" );
//  fprintf( pFile, "edge [dir = none];\n" );
    fprintf( pFile, "edge [dir = back];\n" );
    fprintf( pFile, "\n" );

    // labels on the left of the picture
    fprintf( pFile, "{\n" );
    fprintf( pFile, "  node [shape = plaintext];\n" );
    fprintf( pFile, "  edge [style = invis];\n" );
    fprintf( pFile, "  LevelTitle1 [label=\"\"];\n" );
    fprintf( pFile, "  LevelTitle2 [label=\"\"];\n" );
    // generate node names with labels
    for ( Level = LevelMax; Level >= LevelMin; Level-- )
    {
        // the visible node name
        fprintf( pFile, "  Level%d", Level );
        fprintf( pFile, " [label = " );
        // label name
        fprintf( pFile, "\"" );
        fprintf( pFile, "\"" );
        fprintf( pFile, "];\n" );
    }

    // genetate the sequence of visible/invisible nodes to mark levels
    fprintf( pFile, "  LevelTitle1 ->  LevelTitle2 ->" );
    for ( Level = LevelMax; Level >= LevelMin; Level-- )
    {
        // the visible node name
        fprintf( pFile, "  Level%d",  Level );
        // the connector
        if ( Level != LevelMin )
            fprintf( pFile, " ->" );
        else
            fprintf( pFile, ";" );
    }
    fprintf( pFile, "\n" );
    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );

    // generate title box on top
    fprintf( pFile, "{\n" );
    fprintf( pFile, "  rank = same;\n" );
    fprintf( pFile, "  LevelTitle1;\n" );
    fprintf( pFile, "  title1 [shape=plaintext,\n" );
    fprintf( pFile, "          fontsize=20,\n" );
    fprintf( pFile, "          fontname = \"Times-Roman\",\n" );
    fprintf( pFile, "          label=\"" );
    fprintf( pFile, "%s", "Network structure visualized by ABC" );
    fprintf( pFile, "\\n" );
    fprintf( pFile, "Benchmark \\\"%s\\\". ", pNtk->pName );
    fprintf( pFile, "Time was %s. ",  abc::Extra_TimeStamp() );
    fprintf( pFile, "\"\n" );
    fprintf( pFile, "         ];\n" );
    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );

    // generate statistics box
    fprintf( pFile, "{\n" );
    fprintf( pFile, "  rank = same;\n" );
    fprintf( pFile, "  LevelTitle2;\n" );
    fprintf( pFile, "  title2 [shape=plaintext,\n" );
    fprintf( pFile, "          fontsize=18,\n" );
    fprintf( pFile, "          fontname = \"Times-Roman\",\n" );
    fprintf( pFile, "          label=\"" );
    if ( Abc_NtkObjNum(pNtk) == Vec_PtrSize(vNodes) )
        fprintf( pFile, "The network contains %d logic nodes and %d latches.", Abc_NtkNodeNum(pNtk), Abc_NtkLatchNum(pNtk) );
    else
        fprintf( pFile, "The set contains %d logic nodes and spans %d levels.", NtkCountLogicNodes(vNodes), LevelMax - LevelMin + 1 );
    fprintf( pFile, "\\n" );
    fprintf( pFile, "\"\n" );
    fprintf( pFile, "         ];\n" );
    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );

    // generate the POs
    if ( fHasCos )
    {
        fprintf( pFile, "{\n" );
        fprintf( pFile, "  rank = same;\n" );
        // the labeling node of this level
        fprintf( pFile, "  Level%d;\n",  LevelMax );
        // generate the PO nodes
        Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
        {
            if ( !Abc_ObjIsCo(pNode) )
                continue;
            fprintf( pFile, "  Node%d [label = \"%s%s\"",
                pNode->Id,
                (Abc_ObjIsBi(pNode)? Abc_ObjName(Abc_ObjFanout0(pNode)):Abc_ObjName(pNode)),
                (Abc_ObjIsBi(pNode)? "_in":"") );
            fprintf( pFile, ", shape = %s", (Abc_ObjIsBi(pNode)? "box":"invtriangle") );
            if ( pNode->fMarkB )
                fprintf( pFile, ", style = filled" );
            fprintf( pFile, ", color = coral, fillcolor = coral" );
            fprintf( pFile, "];\n" );
        }
        fprintf( pFile, "}" );
        fprintf( pFile, "\n" );
        fprintf( pFile, "\n" );
    }

    // generate nodes of each rank
    for ( Level = LevelMax - fHasCos; Level >= LevelMin && Level > 0; Level-- )
    {
        fprintf( pFile, "{\n" );
        fprintf( pFile, "  rank = same;\n" );
        // the labeling node of this level
        fprintf( pFile, "  Level%d;\n",  Level );
        Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
        {
            if ( (int)pNode->Level != Level )
                continue;
            if ( Abc_ObjFaninNum(pNode) == 0 )
                continue;

/*
            int SuppSize;
            Vec_Ptr_t * vSupp;
            if ( (int)pNode->Level != Level )
                continue;
            if ( Abc_ObjFaninNum(pNode) == 0 )
                continue;
            vSupp = Abc_NtkNodeSupport( pNtk, &pNode, 1 );
            SuppSize = Vec_PtrSize( vSupp );
            Vec_PtrFree( vSupp );
*/

//            fprintf( pFile, "  Node%d [label = \"%d\"", pNode->Id, pNode->Id );
            if ( Abc_NtkIsStrash(pNtk) ) {}
                //pSopString = "";
            else if ( Abc_NtkHasMapping(pNtk) && fGateNames )
                pSopString = Mio_GateReadName((abc::Mio_Gate_t *)pNode->pData);
            else if ( Abc_NtkHasMapping(pNtk) )
                pSopString = NtkPrintSop(abc::Mio_GateReadSop((abc::Mio_Gate_t *)pNode->pData));
            else
                pSopString = NtkPrintSop((char *)pNode->pData);
            //fprintf( pFile, "  Node%d [label = \"%d\\n%s\"", pNode->Id, pNode->Id, pSopString );
            fprintf( pFile, "  Node%d [label = \"%s\\n%s\"", pNode->Id, Abc_ObjName(pNode), pSopString );
//            fprintf( pFile, "  Node%d [label = \"%d\\n%s\"", pNode->Id,
//                SuppSize,
//                pSopString );

            fprintf( pFile, ", shape = ellipse" );
            if ( pNode->fMarkB )
                fprintf( pFile, ", style = filled" );
            fprintf( pFile, "];\n" );
        }
        fprintf( pFile, "}" );
        fprintf( pFile, "\n" );
        fprintf( pFile, "\n" );
    }

    // generate the PI nodes if any
    if ( LevelMin == 0 )
    {
        fprintf( pFile, "{\n" );
        fprintf( pFile, "  rank = same;\n" );
        // the labeling node of this level
        fprintf( pFile, "  Level%d;\n",  LevelMin );
        // generate the PO nodes
        Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
        {
            if ( !Abc_ObjIsCi(pNode) )
            {
                // check if the costant node is present
                if ( Abc_ObjFaninNum(pNode) == 0 && Abc_ObjFanoutNum(pNode) > 0 )
                {
                    fprintf( pFile, "  Node%d [label = \"Const%d %s\"", pNode->Id, Abc_NtkIsStrash(pNode->pNtk) || Abc_NodeIsConst1(pNode), Abc_ObjName(pNode) );
                    fprintf( pFile, ", shape = ellipse" );
                    if ( pNode->fMarkB )
                        fprintf( pFile, ", style = filled" );
                    fprintf( pFile, ", color = coral, fillcolor = coral" );
                    fprintf( pFile, "];\n" );
                }
                continue;
            }
            fprintf( pFile, "  Node%d [label = \"%s\"",
                pNode->Id,
                (Abc_ObjIsBo(pNode)? Abc_ObjName(Abc_ObjFanin0(pNode)):Abc_ObjName(pNode)) );
            fprintf( pFile, ", shape = %s", (Abc_ObjIsBo(pNode)? "box":"triangle") );
            if ( pNode->fMarkB )
                fprintf( pFile, ", style = filled" );
            fprintf( pFile, ", color = coral, fillcolor = coral" );
            fprintf( pFile, "];\n" );
        }
        fprintf( pFile, "}" );
        fprintf( pFile, "\n" );
        fprintf( pFile, "\n" );
    }

    // generate invisible edges from the square down
    fprintf( pFile, "title1 -> title2 [style = invis];\n" );
    Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
    {
        if ( (int)pNode->Level != LevelMax )
            continue;
        fprintf( pFile, "title2 -> Node%d [style = invis];\n", pNode->Id );
    }
    // generate invisible edges among the COs
    Prev = -1;
    Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
    {
        if ( (int)pNode->Level != LevelMax )
            continue;
        if ( !Abc_ObjIsPo(pNode) )
            continue;
        if ( Prev >= 0 )
            fprintf( pFile, "Node%d -> Node%d [style = invis];\n", Prev, pNode->Id );
        Prev = pNode->Id;
    }

    // generate edges
    Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
    {
        if ( Abc_ObjIsLatch(pNode) )
            continue;
        Abc_ObjForEachFanin( pNode, pFanin, k )
        {
            if ( Abc_ObjIsLatch(pFanin) )
                continue;
            fCompl = 0;
            if ( Abc_NtkIsStrash(pNtk) )
                fCompl = Abc_ObjFaninC(pNode, k);
            // generate the edge from this node to the next
            fprintf( pFile, "Node%d",  pNode->Id );
            fprintf( pFile, " -> " );
            fprintf( pFile, "Node%d",  pFanin->Id );
            fprintf( pFile, " [style = %s", fCompl? "dotted" : "solid" );
//            fprintf( pFile, ", label = \"%c\"", 'a' + k );
            fprintf( pFile, "]" );
            fprintf( pFile, ";\n" );
        }
    }

    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );
    fclose( pFile );

    // unmark the nodes from the set
    Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pNode, i )
        pNode->fMarkC = 0;
    if ( vNodesShow )
        Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodesShow, pNode, i )
            pNode->fMarkB = 0;

    // convert the network back into BDDs if this is how it was
    if ( fHasBdds )
        Abc_NtkSopToBdd(pNtk);
}


char * NtkPrintSop( char * pSop )
{
    static char Buffer[1000];
    char * pGet, * pSet;
    pSet = Buffer;
    for ( pGet = pSop; *pGet; pGet++ )
    {
        if ( *pGet == '\n' )
        {
            *pSet++ = '\\';
            *pSet++ = 'n';
        }
        else
            *pSet++ = *pGet;
    }
    *(pSet-2) = 0;
    return Buffer;
}


int NtkCountLogicNodes(Vec_Ptr_t * vNodes)
{
    abc::Abc_Obj_t * pObj;
    int i, Counter = 0;
    Vec_PtrForEachEntry( abc::Abc_Obj_t *, vNodes, pObj, i )
    {
        if ( !Abc_ObjIsNode(pObj) )
            continue;
        if ( Abc_ObjFaninNum(pObj) == 0 && Abc_ObjFanoutNum(pObj) == 0 )
            continue;
        Counter ++;
    }
    return Counter;
}


void Visualize(Abc_Ntk_t * pAbcNtk, string fileName)
{
    FILE * pFile;
    abc::Abc_Ntk_t * pNtk;
    abc::Abc_Obj_t * pNode;
    abc::Vec_Ptr_t * vNodes;
    int nBarBufs;
    int i;

    assert( abc::Abc_NtkIsStrash(pAbcNtk) || abc::Abc_NtkIsLogic(pAbcNtk) );
    if ( abc::Abc_NtkIsStrash(pAbcNtk) && abc::Abc_NtkGetChoiceNum(pAbcNtk) )
    {
        printf( "Temporarily visualization of AIGs with choice nodes is disabled.\n" );
        return;
    }
    // check that the file can be opened
    if ( (pFile = fopen( const_cast<char *>(fileName.c_str()), "w" )) == nullptr )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", fileName.c_str());
        return;
    }
    fclose( pFile );

    // convert to logic SOP
    pNtk = abc::Abc_NtkDup( pAbcNtk );
    if ( abc::Abc_NtkIsLogic(pNtk) && !abc::Abc_NtkHasMapping(pNtk) )
        abc::Abc_NtkToSop( pNtk, -1, ABC_INFINITY );

    // collect all nodes in the network
    vNodes = abc::Vec_PtrAlloc( 100 );
    Abc_NtkForEachObj( pNtk, pNode, i )
        abc::Vec_PtrPush( vNodes, pNode );
    // write the DOT file
    nBarBufs = pNtk->nBarBufs;
    pNtk->nBarBufs = 0;
    WriteDotNtk( pNtk, vNodes, nullptr, const_cast<char *>(fileName.c_str()), 1, 0 );
    pNtk->nBarBufs = nBarBufs;
    abc::Vec_PtrFree( vNodes );

    abc::Abc_NtkDelete( pNtk );
}
