#!/usr/bin/env groovy

// checkout
def credential_for_checkout = "e9dd3dd6-4c08-4590-b76e-6190e07bbde7"
def giturl_for_checkout = "http://10.8.185.29:20080/mkdir/test_pipeline.git"

def docker_for_compile = "broker_2020_12_2:latest"

def GetRemoteServer(ip){
    def remote = [:]
    remote.name = ip
    remote.host = ip
    remote.port = 22
    remote.allowAnyHosts = true
    withCredentials([usernamePassword(credentialsId: 'e083f880-0c50-4e54-a822-e28438b0cf0a', passwordVariable: 'password', usernameVariable: 'userName')]) {
        remote.user = "${userName}"
        remote.password = "${password}"
    }
    return remote
}

pipeline {
    agent any

    stages {
        /*stage('Hello') {
            agent {
                docker { image 'maven:3-alpine' }
            }
            steps {
                echo "Hello Mr.${username}"
                sh 'mvn --version'
            }
        }*/
        
        stage('Checkout from git') {
            steps {
                echo env.WORKSPACE
                echo env.JOB_NAME
                echo env.JOB_BASE_NAME
                echo env.BUILD_TAG
                git credentialsId: "${credential_for_checkout}", url: "${giturl_for_checkout}"
            }
        }
        
        stage('Build') {
            agent {
                docker { 
                    //image 'broker_2020_12_2:latest' 
                    image "${docker_for_compile}"
                }
            }
            steps {
                // compile
                sh label: '', script: '''
                    git submodule init
                    git submodule update
                    mkdir -p demo4quote/cpp/cmake/build
                    pushd demo4quote/cpp/cmake/build
                    cmake ../..
                    make -j
                    popd
                    mkdir -p deploy
                    cp demo4quote/cpp/cmake/build/demo4quote deploy
                    cp demo4quote/cpp/config.json deploy
                '''
                
                sh label: '', script: '''
                    docker build -t $JOB_BASE_NAME/v1 . -f Dockerfile_test 
                    docker login -u admin -p Harbor12345 http://10.8.33.157:10006/
                    docker tag $JOB_BASE_NAME/v1 10.8.33.157:10006/library/$JOB_BASE_NAME:v1
                    docker push 10.8.33.157:10006/library/$JOB_BASE_NAME:v1
                    docker logout
                '''
                // push to habor
                //docker_app = docker.build env.JOB_BASE_NAME/v1 "-f Dockerfile_test ."
                //withDockerRegistry(credentialsId: '747f194f-121f-4a1f-bbbc-63b23f815a3d', url: 'http://10.8.33.157:10006/') {
                //    docker_app.push()
                //}
            }
        }

        stage('Deploy') {
            steps {
                script{
                    rserver = GetRemoteServer('10.8.78.83')
                    sshCommand remote: rserver, command: "sudo docker pull 10.8.33.157:10006/library/$JOB_BASE_NAME:v1 && sudo docker run -d 10.8.33.157:10006/library/$JOB_BASE_NAME:v1"
                }
            }
        }
    }
}
