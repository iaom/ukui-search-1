#include <QFile>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDebug>
#include "file-utils.h"
#include "index-generator.h"
#include "chinesecharacterstopinyin.h"
#include <QtConcurrent>
#include <QFuture>

using namespace std;

#define INDEX_PATH (QStandardPaths::writableLocation(QStandardPaths::HomeLocation)+"/.config/org.ukui/index_data").toStdString()
#define CONTENT_INDEX_PATH (QStandardPaths::writableLocation(QStandardPaths::HomeLocation)+"/.config/org.ukui/content_index_data").toStdString()

static IndexGenerator *global_instance = nullptr;

IndexGenerator *IndexGenerator::getInstance()
{
    if (!global_instance) {
        global_instance = new IndexGenerator;
    }
    return global_instance;
}

bool IndexGenerator::setIndexdataPath()
{
    return true;
}

bool IndexGenerator::creatAllIndex(QList<QVector<QString> > *messageList)
{
    HandlePathList(messageList);
    try
    {
        m_indexer = new Xapian::TermGenerator();
        m_indexer->set_database(*m_datebase_path);
        //可以实现拼写纠正
//        m_indexer->set_flags(Xapian::TermGenerator::FLAG_SPELLING);
        m_indexer->set_stemming_strategy(Xapian::TermGenerator::STEM_SOME);

        int count =0;
        for(int i = 0;i < m_doc_list_path->size(); i++)
        {
            insertIntoDatabase(m_doc_list_path->at(i));

            if(++count == 9999)
            {
                count = 0;
                m_datebase_path->commit();
            }
        }
        m_datebase_path->commit();
    }
    catch(const Xapian::Error &e)
    {
        qDebug()<<"creatAllIndex fail!"<<QString::fromStdString(e.get_description());
        return false;
    }
    m_doc_list_path->clear();
    Q_EMIT this->transactionFinished();

    return true;
}

bool IndexGenerator::creatAllIndex(QVector<QString> *messageList)
{
    HandlePathList(messageList);
    return true;

}

IndexGenerator::IndexGenerator(QObject *parent) : QObject(parent)
{
    m_datebase_path = new Xapian::WritableDatabase(INDEX_PATH, Xapian::DB_CREATE_OR_OPEN);
    m_database_content = new Xapian::WritableDatabase(CONTENT_INDEX_PATH, Xapian::DB_CREATE_OR_OPEN);
}

IndexGenerator::~IndexGenerator()
{
}

void IndexGenerator::insertIntoDatabase(Document doc)
{
//    qDebug()<< "--index start--";
    Xapian::Document document = doc.getXapianDocument();
    m_indexer->set_document(document);
//    qDebug()<<doc.getIndexText();

    for(auto i : doc.getIndexText()){
        m_indexer->index_text(i.toStdString());
    }

    Xapian::docid innerId= m_datebase_path->replace_document(doc.getUniqueTerm(),document);
//    qDebug()<<"replace doc docid="<<static_cast<int>(innerId);
//    qDebug()<< "--index finish--";
    return;
}

void IndexGenerator::HandlePathList(QList<QVector<QString>> *messageList)
{
    qDebug()<<"Begin HandlePathList!";
    qDebug()<<messageList->size();
//    qDebug()<<QString::number(quintptr(QThread::currentThreadId()));
    QFuture<Document> future = QtConcurrent::mapped(*messageList,&IndexGenerator::GenerateDocument);

    future.waitForFinished();

    QList<Document> docList = future.results();
    m_doc_list_path = new QList<Document>(docList);
    qDebug()<<m_doc_list_path->size();

    qDebug()<<"Finish HandlePathList!";
    return;
}

void IndexGenerator::HandlePathList(QVector<QString> *messageList)
{
    qDebug()<<"Begin HandlePathList for content index!";
    qDebug()<<messageList->size();
//    qDebug()<<QString::number(quintptr(QThread::currentThreadId()));
    QFuture<Document> future = QtConcurrent::mapped(*messageList,&IndexGenerator::GenerateContentDocument);

    future.waitForFinished();

    QList<Document> docList = future.results();
    m_doc_list_content = new QList<Document>(docList);
    qDebug()<<m_doc_list_content->size();

    qDebug()<<"Finish HandlePathList for content index!";
    return;

}

Document IndexGenerator::GenerateDocument(const QVector<QString> &list)
{
//    qDebug()<<QString::number(quintptr(QThread::currentThreadId()));
    //0-filename 1-filepathname 2-file or dir
    QString index_text = list.at(0);
    QString sourcePath = list.at(1);   
    index_text = index_text.replace(".","").replace(""," ");
    index_text = index_text.simplified();
    //这个应该是写错了
//    QString pinyin_text = FileUtils::find(index_text.replace(".", "")).replace("", " ");

    //不带多音字版
//    QString pinyin_text = FileUtils::find(QString(list.at(0)).replace(".","")).replace("", " ").simplified();

    //多音字版
    //现加入首字母
    QStringList pinyin_text_list = FileUtils::findMultiToneWords(QString(list.at(0)).replace(".",""));
    for (QString& i : pinyin_text_list){
        i.replace("", " ");
    }

    QString uniqueterm = QString::fromStdString(FileUtils::makeDocUterm(sourcePath));
//    QString uniqueterm1 = QString::fromStdString(QCryptographicHash::hash(sourcePath.toUtf8(),QCryptographicHash::Md5).toStdString());
/*--------------------------------------------------------------------*/
    //QByteArray 和  QString 之间会进行隐式转换，造成字符串被截断等意想不到的后果！！！！！！！ zpf
    //    if(uniqueterm1!=uniqueterm){
//        qDebug()<<"-----------------------------------------start";
//        qDebug()<<uniqueterm1;
//        qDebug()<<uniqueterm;
//        qDebug()<<"------------------------------------------finish";
//    }
/*--------------------------------------------------------------------*/
    Document doc;
    doc.setData(sourcePath);
    doc.setUniqueTerm(uniqueterm);
    doc.addValue(list.at(2));
    if(list.at(2) == QString("1"))
        qDebug()<<"value!!!"<<list.at(2);
    QStringList temp;
    temp.append(index_text);
    temp.append(pinyin_text_list);
    doc.setIndexText(temp);
//    doc.setIndexText(QStringList()<<index_text<<pinyin_text);
//    doc.setIndexText(QStringList()<<index_text);
    return doc;

}

Document IndexGenerator::GenerateContentDocument(const QString &path)
{
    //构造文本索引的document
    FileReader::getTextContent(path);
    QString uniqueterm = QString::fromStdString(FileUtils::makeDocUterm(path));
    Document doc;
    doc.setData(path);
    doc.setUniqueTerm(uniqueterm);
    return doc;


}

bool IndexGenerator::isIndexdataExist()
{

//    Xapian::Database db(m_index_data_path->toStdString());


}

QStringList IndexGenerator::IndexSearch(QString indexText)
{
    QStringList searchResult;
    try
    {
        qDebug()<<"--search start--";

        Xapian::Database db(INDEX_PATH);
        Xapian::Enquire enquire(db);
        Xapian::QueryParser qp;
        qp.set_default_op(Xapian::Query::OP_PHRASE);
        qp.set_database(db);
        auto userInput = indexText;

        std::string queryStr = indexText.replace(""," ").toStdString();
//        std::string s =db.get_spelling_suggestion(queryStr,10);
//        qDebug()<<"spelling_suggestion!"<<QString::fromStdString(s);

        qDebug()<<"queryStr!"<<QString::fromStdString(queryStr);
        //Creat a query
        Xapian::Query queryPhrase = qp.parse_query(queryStr,Xapian::QueryParser::FLAG_PHRASE);
        std::vector<Xapian::Query> v;
        for(int i=0;i<userInput.size();i++)
        {
            v.push_back(Xapian::Query(QString(userInput.at(i)).toStdString()));
            qDebug()<<userInput.at(i);
            qDebug()<<QString::fromStdString(Xapian::Query(QString(userInput.at(i)).toStdString()).get_description());
        }
        Xapian::Query queryNear =Xapian::Query(Xapian::Query::OP_NEAR, v.begin(), v.end());
        Xapian::Query query = Xapian::Query(Xapian::Query::OP_AND,queryNear,queryPhrase);

        qDebug()<<QString::fromStdString(query.get_description());
        enquire.set_query(query);

        Xapian::MSet result = enquire.get_mset(0, 9999);
        qDebug()<< "find results count=" <<static_cast<int>(result.get_matches_estimated());

//        QStringList *pathTobeDelete = new QStringList;

        //get search result
        for (auto it = result.begin(); it != result.end(); ++it) {
            Xapian::Document doc = it.get_document();
            std::string data = doc.get_data();
            Xapian::weight docScoreWeight = it.get_weight();
            Xapian::percent docScorePercent = it.get_percent();
            QFileInfo *info = new QFileInfo(QString::fromStdString(data));

            if(!info->exists())
            {
//                pathTobeDelete->append(QString::fromStdString(data));
                qDebug()<<QString::fromStdString(data)<<"is not exist!!";
            }
            else
            {
                searchResult.append(QString::fromStdString(data));
            }

            qDebug()<< "doc="<< QString::fromStdString(data) << ",weight=" <<docScoreWeight << ",percent=" << docScorePercent;
        }
//        //Delete those path doc which is not already exist.
//        if(!pathTobeDelete->isEmpty())
//            deleteAllIndex(pathTobeDelete);

        qDebug()<< "--search finish--";
    }
    catch(const Xapian::Error &e)
    {
        qDebug() <<QString::fromStdString(e.get_description());
    }
    return searchResult;
}

bool IndexGenerator::deleteAllIndex(QStringList *pathlist)
{
    QStringList *list = pathlist;
    if(list->isEmpty())
        return true;
    for(int i = 0;i<list->size();i++)
    {
        QString doc = list->at(i);
        std::string uniqueterm = FileUtils::makeDocUterm(doc);
        try
        {
            qDebug()<<"--delete start--";
            m_datebase_path->delete_document(uniqueterm);
            qDebug()<<"delete md5"<<QString::fromStdString(uniqueterm);
            m_datebase_path->commit();
            qDebug()<< "--delete finish--";
        }
        catch(const Xapian::Error &e)
        {
            qDebug() <<QString::fromStdString(e.get_description());
            return false;
        }
    }
    Q_EMIT this->transactionFinished();
    return true;
}

