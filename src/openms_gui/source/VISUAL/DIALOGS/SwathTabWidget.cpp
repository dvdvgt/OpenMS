// --------------------------------------------------------------------------
//                   OpenMS -- Open-Source Mass Spectrometry
// --------------------------------------------------------------------------
// Copyright The OpenMS Team -- Eberhard Karls University Tuebingen,
// ETH Zurich, and Freie Universitaet Berlin 2002-2020.
//
// This software is released under a three-clause BSD license:
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//  * Neither the name of any author or any participating institution
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// For a full list of authors, refer to the file AUTHORS.
// --------------------------------------------------------------------------
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL ANY OF THE AUTHORS OR THE CONTRIBUTING
// INSTITUTIONS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// --------------------------------------------------------------------------
// $Maintainer: Chris Bielow $
// $Authors: Chris Bielow $
// --------------------------------------------------------------------------

#include <OpenMS/VISUAL/DIALOGS/SwathTabWidget.h>
#include <ui_SwathTabWidget.h>

#include <OpenMS/FORMAT/ParamXMLFile.h>
#include <OpenMS/SYSTEM/File.h>
#include <OpenMS/VISUAL/DIALOGS/PythonModuleRequirement.h>
#include <OpenMS/VISUAL/DIALOGS/TOPPASInputFilesDialog.h>

#include <qprocess.h>

using namespace std;

namespace OpenMS
{
  namespace Internal
  {
    SwathTabWidget::SwathTabWidget(QWidget* parent) :
        QTabWidget(parent),
        ui(new Ui::SwathTabWidget)
    {
        ui->setupUi(this);
        
        auto py_selector = (PythonSelector*)ui->py_selector;

        auto py_pyprophet = (PythonModuleRequirement*)ui->py_pyprophet;
        py_pyprophet->setRequiredModules({"pyprophet", "stats"});
        py_pyprophet->setFreeText("In order to run PyProphet after OpenSWATH, the above modules need to be installed\n" \
                                  "Once they are available, the 'pyProphet' tab will become active and configurable.");
        py_pyprophet->setTitle("External: PyProphet tool");
        connect(py_selector, &PythonSelector::valueChanged, py_pyprophet, &PythonModuleRequirement::validate);
        
        // call once to update py_pyprophet canvas 
        // alternative: load latest data from .ini and set py_selector (will update py_pyprophet via above signal/slot)
        py_pyprophet->validate(py_selector->getLastPython().toQString());

        ui->input_tr->setFileFormatFilter("Transition sqLite file (*.pqp)");
        ui->input_iRT->setFileFormatFilter("Transition sqLite file (*.pqp)");

        // create a default config from OpenSwathWorkflow
        String executable = File::getExecutablePath() + "OpenSwathWorkflow";
        String tmp_file = File::getTemporaryFile();
        QProcess qp;
        qp.start(executable.toQString(), QStringList() << "-write_ini" << tmp_file.toQString());
        qp.waitForFinished();
        
        ParamXMLFile().load(tmp_file, swath_param_);
        swath_param_ = swath_param_.copy("OpenSwathWorkflow:1:", true);
        // parameters to show:
        StringList extract = {"mz_extraction_window", "rt_extraction_window", "threads"};
        
        for (const auto& name : extract) swath_param_wizard_.setValue(name, ""); // create a dummy param, just so we can use ::copySubset
        swath_param_wizard_ = swath_param_.copySubset(swath_param_wizard_);
                
        ui->list_editor->load(swath_param_wizard_);
    }

    SwathTabWidget::~SwathTabWidget()
    {
        delete ui;
    }

    void SwathTabWidget::on_pushButton_clicked()
    {
        TOPPASInputFilesDialog tifd({}, "", 0);
        tifd.exec();
    }

    void SwathTabWidget::on_run_swath_clicked()
    {

    }

    void SwathTabWidget::on_edit_advanced_parameters_clicked()
    {
      Param tmp_param = swath_param_;
      ui->list_editor->store(); // update 'swath_param_wizard_' in ParamEditor
      tmp_param.update(swath_param_wizard_, false); // update with selected params in the Wizard itself
      StringList to_remove;
      for (Param::ParamIterator it = tmp_param.begin(); it != tmp_param.end(); ++it)
      {
        if (it->tags.count("input file") || it->tags.count("output file"))
        {
          to_remove.push_back(it->name); // do not remove right away.. does not work
        }
      }
      for (const auto& p : to_remove) 
      {
        tmp_param.remove(p);
        if (tmp_param.exists(p + "_type")) tmp_param.remove(p + "_type"); // for good measure of related input/output parameters
      }

      tmp_param.remove("tr");
      String executable = File::getExecutablePath() + "INIFIleEditor";
      String tmp_file = File::getTemporaryFile();
      ParamXMLFile().store(tmp_file, tmp_param);
      QProcess qp;
      qp.start(executable.toQString(), QStringList() << tmp_file.toQString());
      ui->run->setEnabled(false); // grey out the Wizard until INIFileEditor returns...
      qp.waitForFinished(-1);
      ui->run->setEnabled(true);
      ParamXMLFile().load(tmp_file, tmp_param);
      swath_param_.update(tmp_param, false);

    }
  }   //namespace Internal
} //namspace OpenMS




