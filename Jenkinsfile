pipeline {
  agent any
  stages {
    stage('Build') {
      steps {
        dir ('test') {
          bat 'run_tests.bat'
        }
      }
    }

  }
}