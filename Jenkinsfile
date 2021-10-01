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
  post {
    success {
      archiveArtifacts artifacts: 'build/cconstruct.h', fingerprint: false
    }
  }
}