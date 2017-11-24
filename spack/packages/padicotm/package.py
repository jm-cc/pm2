from spack import *
import platform

class Padicotm(AutotoolsPackage):
    """PadicoTM communication framework and launcher"""
    homepage = "http://pm2.gforge.inria.fr/PadicoTM/"

    version('trunk', svn='https://scm.gforge.inria.fr/anonscm/svn/padico/PadicoTM/trunk/PadicoTM/PadicoTM')

    resource(
        name='building-tools',
        svn='https://scm.gforge.inria.fr/anonscm/svn/padico/PadicoTM/trunk/PadicoTM/building-tools',
        destination='.'
    )
    
    variant('debug', default=False, description='Build in debug mode')
    variant('optimize', default=True, description='Build in optimized mode')
    variant('pioman', default=True, description='Build with pioman')

    depends_on('nmad')
    depends_on('puk')
    depends_on('pioman', when='+pioman')
    depends_on('pkgconfig')
    depends_on('autoconf')

    build_directory = 'padicotm-build'
    
    def configure_args(self):
        spec = self.spec

        config_args = [
            '--without-pukabi',
            ]
        
        config_args.extend([
            "--%s-debug"         % ('enable' if '+debug'     in spec else 'disable'),
            "--%s-optimize"      % ('enable' if '+optimize' in spec else 'disable'),
        ])

        return config_args
    
    def autoreconf(self, spec, prefix):
        autogen = Executable("./autogen.sh")
        autogen()
            
        
